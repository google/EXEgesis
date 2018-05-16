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

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "base/stringprintf.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/registers.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/bits.h"
#include "exegesis/util/category_util.h"
#include "exegesis/util/status_util.h"
#include "exegesis/x86/instruction_set_utils.h"
#include "glog/logging.h"
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
  const Status error = InvalidArgumentError(absl::StrCat(
      error_message, "\nInstruction:\n", instruction.DebugString()));
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

namespace {

// Checks that the combination of the name of an operand and its tags is valid.
// We allow the following combinations:
// - non-empty operand name + zero or more tags = a "normal" operand with tags,
// - empty operand name + one or more tags = a "pseudoperand" tag.
// Tag names must always be non-empty.
// TODO(ondrasej): Add more specific checks. Possible ideas:
// - opmaks register tags seem to be always attached to an operand name,
// - embedded rounding modes seem to always be pseudo-operands.
bool OperandNameAndTagsAreValid(const InstructionOperand& operand) {
  if (operand.name().empty() && operand.tags().empty()) return false;
  for (const InstructionOperand::Tag& tag : operand.tags()) {
    if (tag.name().empty()) return false;
  }
  return true;
}

// Returns true if the instruction is one of the XSAVE or XRSTOR instructions.
// The instructions are identified by their mnemonics.
bool IsXsaveOrXrstor(const InstructionProto& instruction) {
  const std::string& mnemonic = instruction.vendor_syntax().mnemonic();
  return absl::StartsWith(mnemonic, "XSAVE") ||
         absl::StartsWith(mnemonic, "XRSTOR");
}

// Returns true if the instruction is an x87 FPU instruction. The FPU
// instructions are identified by their mnemonics.
bool IsX87FpuInstruction(const InstructionProto& instruction) {
  return ContainsKey(GetX87FpuInstructionMnemonics(),
                     instruction.vendor_syntax().mnemonic());
}

Status CheckOperands(const InstructionProto& instruction,
                     absl::string_view format_name,
                     const InstructionFormat& format) {
  Status status;
  for (const InstructionOperand& operand : format.operands()) {
    if (!OperandNameAndTagsAreValid(operand)) {
      LogErrorAndUpdateStatus(absl::StrCat("Operand name or tags in ",
                                           format_name, " are not valid"),
                              instruction, &status);
    }
    const bool is_pseudo_operand = operand.name().empty();
    if (is_pseudo_operand) {
      if (operand.encoding() !=
          InstructionOperand::X86_STATIC_PROPERTY_ENCODING) {
        LogErrorAndUpdateStatus(
            absl::StrCat("Encoding of a pseudo-operand in ", format_name,
                         " is not X86_STATIC_PROPERTY_ENCODING"),
            instruction, &status);
      }
      if (operand.addressing_mode() != InstructionOperand::NO_ADDRESSING) {
        LogErrorAndUpdateStatus(
            absl::StrCat("Addressing mode of a pseudo-operand in ", format_name,
                         " is not NO_ADDRESSING"),
            instruction, &status);
      }
      if (operand.usage() != InstructionOperand::USAGE_READ) {
        LogErrorAndUpdateStatus(absl::StrCat("Usage of a pseudo-operand in ",
                                             format_name, " is not USAGE_READ"),
                                instruction, &status);
      }
    } else {
      if (operand.encoding() == InstructionOperand::ANY_ENCODING) {
        LogErrorAndUpdateStatus(
            absl::StrCat("Operand encoding in ", format_name, " is not set"),
            instruction, &status);
      }
      // NOTE(ondrasej): After running AddAlternatives on the instruction set,
      // all operands should have a more specific addressing mode, e.g.
      // DIRECT_ADDRESSING or INDIRECT_ADDRESSING. Finding ANY_ADDRESSING_MODE
      // means that we're missing some rewriting rules in AddAlternatives.
      if (operand.addressing_mode() ==
          InstructionOperand::ANY_ADDRESSING_MODE) {
        LogErrorAndUpdateStatus(
            absl::StrCat("Addressing mode in ", format_name, " is not set"),
            instruction, &status);
      }
      if (InCategory(operand.addressing_mode(),
                     InstructionOperand::DIRECT_ADDRESSING)) {
        // NOTE(ondrasej): Register class is defined only for direct addressing.
        if (operand.register_class() == RegisterProto::INVALID_REGISTER_CLASS) {
          LogErrorAndUpdateStatus(
              absl::StrCat("Register class in ", format_name, " is not set"),
              instruction, &status);
        }
      }
      // In certain cases, the value size in bits is not well defined.
      const bool value_size_is_undefined =
          // Operands with LOAD_EFFECTIVE_ADDRESS addressing only compute the
          // address, but do not access the value at that address.
          InCategory(operand.addressing_mode(),
                     InstructionOperand::LOAD_EFFECTIVE_ADDRESS) ||
          // Operands with NO_ADDRESSIND and IMPLICIT_ENCODING are implicit
          // immediate values. Instructions using them are usually special cases
          // of a more generic instruction, where the corresponding value comes
          // from a "true" operand. However, the assembler still requires that
          // the value of the operand is entered.
          (InCategory(operand.addressing_mode(),
                      InstructionOperand::NO_ADDRESSING) &&
           InCategory(operand.encoding(),
                      InstructionOperand::IMPLICIT_ENCODING)) ||
          // The VSIB addressing mode is used by the scatter/gather
          // instructions. In principle, these instructions access memory in
          // different locations based on the values of the indices and,
          // optionally, a mask.
          InCategory(operand.addressing_mode(),
                     InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB) ||
          // The size of the operands of the XSAVE*/XRSTOR* instructions depends
          // on the bitmask passed to the instruction.
          IsXsaveOrXrstor(instruction);
      if (!value_size_is_undefined) {
        if (operand.value_size_bits() == 0) {
          LogErrorAndUpdateStatus(
              absl::StrCat("Value size bits in ", format_name, " is not set"),
              instruction, &status);
        }
      }
      if (!IsX87FpuInstruction(instruction)) {
        // We skip this check for floating point stack registers, because they
        // are not specified in the SDM (as of May 2018). Moreover, the actual
        // read/write semantics are not well defined, because many of the
        // instructions implicitly modify _all_ registers on the stack by
        // pushing or popping values.
        if (operand.usage() == InstructionOperand::USAGE_UNKNOWN) {
          LogErrorAndUpdateStatus(
              absl::StrCat("Operand usage in ", format_name, " is not set"),
              instruction, &status);
        }
      }
    }
    // TODO(ondrasej): As of 2017-11-02, we're not filling the data_type
    // field when importing data from the Intel SDM. Do the same checks for
    // these instructions when we start supporting them.
  }
  return status;
}

}  // namespace

Status CheckOperandInfo(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kVendorSyntax[] = "InstructionProto.vendor_syntax";
  Status status;
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    // TODO(ondrasej): As of 2017-11-02, we fill the detailed fields only in
    // vendor_syntax. We should extend these checks to other fields as well once
    // we start populating them.
    UpdateStatus(&status, CheckOperands(instruction, kVendorSyntax,
                                        instruction.vendor_syntax()));
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(CheckOperandInfo, 10000);

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
