// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "exegesis/x86/cleanup_instruction_set_checks.h"

#include <cstdint>
#include <unordered_set>

#include "base/stringprintf.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/bits.h"
#include "exegesis/util/status_util.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

constexpr const uint8_t kModRmDirectAddressing = 0x3;

void LogErrorAndUpdateStatus(const std::string& error_message,
                             const InstructionProto& instruction,
                             Status* status) {
  const Status error = InvalidArgumentError(
      StrCat(error_message, "\nInstruction:\n", instruction.DebugString()));
  // In case of the check cleanups, we want to print as many errors as possible
  // to make their analysis easier.
  LOG(WARNING) << error;
  UpdateStatus(status, error);
}

// TODO(ondrasej): Delete following helpers once instruction_encoding.h has been
// open sourced.
// Returns mod field of a ModR/M byte. Result is shifted to the right so that
// the LSB of the field is the LSB of returned value.
inline uint8_t GetModRmModBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 6, 8);
}

// Returns reg field of a ModR/M byte. Result is shifted to the right so that
// the LSB of the field is the LSB of returned value.
inline uint8_t GetModRmRegBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 3, 6);
}

// Returns rm field of a ModR/M byte. Result is shifted to the right so that the
// LSB of the field is the LSB of returned value.
inline uint8_t GetModRmRmBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 0, 3);
}

uint8_t UsesDirectAddressingInModRm(const InstructionProto& instruction) {
  const EncodingSpecification& encoding_specification =
      instruction.x86_encoding_specification();
  CHECK(encoding_specification.modrm_usage() !=
        EncodingSpecification::NO_MODRM_USAGE)
      << "Instruction does not have ModR/M byte " << instruction.DebugString();

  // Check if instruction uses DIRECT_ADDRESSING.
  for (const InstructionOperand& operand :
       instruction.vendor_syntax().operands()) {
    if (operand.encoding() == InstructionOperand::MODRM_RM_ENCODING &&
        operand.addressing_mode() == InstructionOperand::DIRECT_ADDRESSING) {
      return true;
    }
  }

  return false;
}

bool IsSpecialCaseOfInstruction(const InstructionProto& general,
                                const EncodingSpecification& special_case) {
  const EncodingSpecification& general_encoding =
      general.x86_encoding_specification();

  // If general's opcode is not a prefix of the special case's opcode, it cannot
  // be a special case.
  if (general_encoding.opcode() != (special_case.opcode() >> 8)) {
    return false;
  }
  // If general doesn't use ModR/M encoding, then there is definitely ambiguity.
  if (general_encoding.modrm_usage() == EncodingSpecification::NO_MODRM_USAGE) {
    return true;
  }

  const bool general_uses_direct_addressing =
      UsesDirectAddressingInModRm(general);
  const bool special_uses_direct_addressing =
      GetModRmModBits(special_case.opcode()) == kModRmDirectAddressing;
  // Make sure they both have same addressing type, direct or indirect.
  if (general_uses_direct_addressing != special_uses_direct_addressing) {
    return false;
  }
  // If there is an opcode extension in modrm field then we need to make sure
  // reg fields have the same value.
  if (general_encoding.modrm_usage() ==
          EncodingSpecification::OPCODE_EXTENSION_IN_MODRM &&
      general_encoding.modrm_opcode_extension() !=
          GetModRmRegBits(special_case.opcode())) {
    return false;
  }

  return true;
}

}  // namespace

Status CheckOpcodeFormat(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static constexpr uint32_t kOpcodeUpperBytesMask = 0xffffff00;
  const std::unordered_set<uint32_t> kAllowedUpperBytes = {0, 0x0f00, 0x0f3800,
                                                           0x0f3a00};
  // Also check that there are no opcodes that would be a prefix of a longer
  // multi-byte opcode.
  const std::unordered_set<uint32_t> kForbiddenOpcodes = {0x0f, 0x0f38, 0x0f3a};
  Status status = OkStatus();
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    if (!instruction.has_x86_encoding_specification()) {
      LogErrorAndUpdateStatus(
          "The instruction does not have an encoding specification.",
          instruction, &status);
      continue;
    }
    const EncodingSpecification& encoding_specification =
        instruction.x86_encoding_specification();
    const uint32_t opcode = encoding_specification.opcode();
    const uint32_t opcode_upper_bytes = opcode & kOpcodeUpperBytesMask;
    if (!ContainsKey(kAllowedUpperBytes, opcode_upper_bytes)) {
      LogErrorAndUpdateStatus(
          StringPrintf("Invalid opcode upper bytes: %x", opcode_upper_bytes),
          instruction, &status);
      continue;
    }
    if (ContainsKey(kForbiddenOpcodes, opcode)) {
      LogErrorAndUpdateStatus(StringPrintf("Invalid opcode: %x", opcode),
                              instruction, &status);
      continue;
    }
  }
  return status;
}
// TODO(ondrasej): Add this transform to the default pipeline when all problems
// it finds are resolved.

Status CheckSpecialCaseInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  std::unordered_map<uint32_t, std::vector<const InstructionProto*>>
      instructions_by_opcode;
  Status status = OkStatus();

  for (const InstructionProto& instruction : instruction_set->instructions()) {
    if (!instruction.has_x86_encoding_specification()) {
      LogErrorAndUpdateStatus(
          "The instruction does not have an encoding specification.",
          instruction, &status);
      continue;
    }
    const EncodingSpecification& encoding_specification =
        instruction.x86_encoding_specification();
    const uint32_t opcode = encoding_specification.opcode();
    // Build an index for instructions based on opcodes.
    instructions_by_opcode[opcode].push_back(&instruction);
  }
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    const EncodingSpecification& encoding_specification =
        instruction.x86_encoding_specification();
    const uint32_t opcode = encoding_specification.opcode();
    if (opcode <= 0xff) continue;
    const std::vector<const InstructionProto*>* const candidates =
        FindOrNull(instructions_by_opcode, opcode >> 8);
    if (candidates != nullptr) {
      // Test coverage against all instructions whose opcode is a prefix of this
      // instruciton.
      for (const InstructionProto* candidate : *candidates) {
        if (IsSpecialCaseOfInstruction(
                *candidate, instruction.x86_encoding_specification())) {
          LogErrorAndUpdateStatus(
              StringPrintf("Opcode is ambigious: %x\n%s", opcode,
                           candidate->DebugString().c_str()),
              instruction, &status);
        }
      }
    }
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(CheckSpecialCaseInstructions, 10000);

}  // namespace x86
}  // namespace exegesis
