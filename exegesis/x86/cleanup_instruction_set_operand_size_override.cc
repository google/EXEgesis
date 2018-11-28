// Copyright 2016 Google Inc.
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

#include "exegesis/x86/cleanup_instruction_set_operand_size_override.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/base/prettyprint.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/x86/cleanup_instruction_set_utils.h"
#include "exegesis/x86/encoding_specification.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

// Mnemonics of 16-bit string instructions that take no operands.
const char* const k16BitInstructionsWithImplicitOperands[] = {
    "CMPSW", "CBW",   "CWD",  "INSW",  "IRET",  "LODSW",
    "MOVSW", "OUTSW", "POPF", "PUSHF", "SCASW", "STOSW"};

LegacyEncoding::PrefixUsage GetOperandSizeOverrideUsage(
    const InstructionProto& instruction) {
  DCHECK(instruction.has_x86_encoding_specification());
  const EncodingSpecification& encoding_specification =
      instruction.x86_encoding_specification();
  DCHECK(encoding_specification.has_legacy_prefixes());
  const auto& legacy_prefixes = encoding_specification.legacy_prefixes();
  return legacy_prefixes.operand_size_override_prefix();
}

void SetOperandSizeOverrideUsage(InstructionProto* instruction,
                                 LegacyEncoding::PrefixUsage usage) {
  DCHECK(instruction != nullptr);
  DCHECK(instruction->has_x86_encoding_specification());
  EncodingSpecification* const encoding_specification =
      instruction->mutable_x86_encoding_specification();
  LegacyPrefixEncodingSpecification* const legacy_prefixes =
      encoding_specification->mutable_legacy_prefixes();
  legacy_prefixes->set_operand_size_override_prefix(usage);
}

}  // namespace

Status AddOperandSizeOverrideToInstructionsWithImplicitOperands(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_set<std::string> string_instructions(
      std::begin(k16BitInstructionsWithImplicitOperands),
      std::end(k16BitInstructionsWithImplicitOperands));
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    if (ContainsVendorSyntaxMnemonic(string_instructions, instruction)) {
      AddOperandSizeOverrideToInstructionProto(&instruction);
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    AddOperandSizeOverrideToInstructionsWithImplicitOperands, 3000);

Status AddOperandSizeOverrideToSpecialCaseInstructions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_set<std::string> k16BitOperands = {"r16", "r/m16"};
  // One of the operands of the instructions gives away its 16-bit-ness;
  // unfortunately, the position of these operands may differ from instruction
  // to instruction. In this map, we keep the list of affected binary encodings,
  // and the index of the operand that can be used to find the 16-bit version.
  const absl::flat_hash_map<std::string, int> kOperandIndex = {
      {"0F 01 /4", 0},        // SMSW r/m16; SMSW r32/m16
      {"0F B2 /r", 0},        // LSS r16,m16:16; LSS r32,m16:32
      {"0F B2 /r", 0},        // LSS r16,m16:16; LSS r32,m16:32
      {"0F B4 /r", 0},        // LFS r16,m16:16; LFS r32,m16:32
      {"0F B5 /r", 0},        // LGS r16,m16:16; LGS r32,m16:32
      {"50+rw", 0},           // PUSH r16; PUSH r64
      {"58+ rw", 0},          // POP r16; POP r64
      {"62 /r", 0},           // BOUND r16,m16&16; BOUND r32,m32&32
      {"8F /0", 0},           // POP r/m16; POP r/m64
      {"C4 /r", 0},           // LES r16,m16:16; LES r32,m16:32
      {"C5 /r", 0},           // LDS r16,m16:16; LDS r32,m16:32
      {"F2 0F 38 F1 /r", 1},  // CRC32 r32,r/m16; CRC32 r32,r/m32
      {"FF /6", 0},           // PUSH r/m16; PUSH r/m64
  };
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    int operand_index = -1;
    if (gtl::FindCopy(kOperandIndex, instruction.raw_encoding_specification(),
                      &operand_index)) {
      const InstructionFormat& vendor_syntax =
          GetUniqueVendorSyntaxOrDie(instruction);
      if (operand_index >= vendor_syntax.operands_size()) {
        return InvalidArgumentError(
            absl::StrCat("Unexpected number of operands of instruction: ",
                         instruction.raw_encoding_specification()));
      }
      // We can't rely just on the information in value_size_bits, because
      // technically, even the 32 or 64-bit versions of the instruction often
      // use a 16-bit value, and just leave the other bits undefined (or
      // zeroed). Instead, we need to look at the string representation of the
      // type of the operand.
      if (k16BitOperands.contains(
              vendor_syntax.operands(operand_index).name())) {
        AddOperandSizeOverrideToInstructionProto(&instruction);
      }
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    AddOperandSizeOverrideToSpecialCaseInstructions, 3000);

namespace {

// Returns true if 'instruction' has an operand of the given size.
bool HasDataOperandOfSize(int size, const InstructionProto& instruction) {
  const InstructionFormat& vendor_syntax =
      GetVendorSyntaxWithMostOperandsOrDie(instruction);
  return std::any_of(vendor_syntax.operands().begin(),
                     vendor_syntax.operands().end(),
                     [size](const InstructionOperand& operand) {
                       return operand.value_size_bits() == size;
                     });
}

// Prints a string that contains the vendor syntax of all instructions in
// 'instructions' in a human-readable format.
std::string FormatAllInstructions(
    const std::vector<InstructionProto*>& instructions) {
  std::vector<std::string> vendor_syntaxes;
  vendor_syntaxes.reserve(instructions.size());
  for (const InstructionProto* const instruction : instructions) {
    vendor_syntaxes.push_back(PrettyPrintSyntaxes(
        instruction->vendor_syntax(),
        PrettyPrintOptions().WithVendorSyntaxesOnOneLine(true)));
  }
  return absl::StrJoin(vendor_syntaxes, "; ");
}

}  // namespace

Status AddOperandSizeOverridePrefix(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  absl::node_hash_map<std::string, std::vector<InstructionProto*>>
      instructions_by_raw_encoding_specification;

  // First we cluster instructions by their binary encoding. We ignore the
  // size(s) of immediate values, because their sizes often differ, even though
  // they do not have a relation to the 16/32-bit dichotomy.
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const std::string& raw_encoding_specification =
        instruction.raw_encoding_specification();
    if (raw_encoding_specification.empty()) {
      return InvalidArgumentError(
          absl::StrCat("No binary encoding specification for instruction:\n",
                       instruction.DebugString()));
    }
    if (!instruction.has_x86_encoding_specification()) {
      return FailedPreconditionError(absl::StrCat(
          "Instruction does not have a parsed encoding spcification:\n",
          instruction.DebugString()));
    }
    EncodingSpecification specification =
        instruction.x86_encoding_specification();

    // The instruction has a code offset operand. The size of this offset is
    // controlled by the address size override, not the operand size override.
    // Moreover, there are no instructions that would combine a code offset with
    // other arguments, so we can simply skip them to avoid confusing them with
    // data operands.
    if (specification.code_offset_bytes() > 0) continue;

    // VEX instructions do not suffer from the same 16/32-bit specification
    // problem, so we could just ignore them all.
    if (specification.has_vex_prefix()) continue;

    // Remove information about immediate values from the encoding, and then
    // index the instructions by the serialized version of the proto.
    specification.clear_immediate_value_bytes();
    const std::string serialized_specification =
        specification.SerializeAsString();
    instructions_by_raw_encoding_specification[serialized_specification]
        .push_back(&instruction);
  }

  // Inspect all instruction groups and add the operand size override prefix if
  // needed.
  for (const auto& bucket : instructions_by_raw_encoding_specification) {
    const std::vector<InstructionProto*>& instructions = bucket.second;

    // If there is only one instruction in the group, it probably means that it
    // is OK (or the Intel manual forgot to list the instruction as both 16- and
    // 32-bit).
    if (instructions.size() <= 1) continue;

    // Try to find the 16-bit and the 32-bit versions of the instruction.
    std::vector<InstructionProto*> instructions_16bit;
    std::vector<InstructionProto*> instructions_32bit;
    for (InstructionProto* const instruction : instructions) {
      // Some instructions have both 16-bit and 32-bit operands. This happens
      // for example in case of IO port instructions - the port number is a
      // 16-bit register, while the value written to it may be either 16-bit or
      // 32-bit. We mark an instruction as 16-bit, only if it does not have a
      // 32-bit operand, to avoid adding these 16/32-bit instructions to both
      // groups.
      if (HasDataOperandOfSize(32, *instruction)) {
        instructions_32bit.push_back(instruction);
      } else if (HasDataOperandOfSize(16, *instruction)) {
        instructions_16bit.push_back(instruction);
      }
    }

    if (instructions_16bit.empty() || instructions_32bit.empty()) {
      if (!instructions_16bit.empty() || !instructions_32bit.empty()) {
        VLOG(1)
            << "Instruction has multiple versions, but they are not 16- and "
               "32-bit: "
            << instructions[0]->raw_encoding_specification() << " ("
            << FormatAllInstructions(instructions) << ")";
      }
      continue;
    }
    VLOG(1) << "Updating instruction: "
            << instructions[0]->raw_encoding_specification() << " ("
            << FormatAllInstructions(instructions) << ")";
    for (InstructionProto* const instruction : instructions_16bit) {
      AddOperandSizeOverrideToInstructionProto(instruction);
    }
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddOperandSizeOverridePrefix, 5000);

Status AddOperandSizeOverridePrefixUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  AddPrefixUsageToLegacyInstructions(GetOperandSizeOverrideUsage,
                                     SetOperandSizeOverrideUsage,
                                     instruction_set);
  return OkStatus();
}
// This transform must run after all mandatory operand size overrides are added
// to the encoding specifications.
REGISTER_INSTRUCTION_SET_TRANSFORM(AddOperandSizeOverridePrefixUsage, 5010);

Status AddOperandSizeOverrideVersionForSpecialCaseInstructions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_set<std::string> k16BitOperands = {"r16", "r/m16"};
  // Following instructions can have operand size override prefix or not,
  // because they implicitly operate on 16 bit data. Since, it is up to compiler
  // to add the prefix or not we are adding both versions to be able to match
  // them in any case.
  const absl::flat_hash_map<std::string, int> kOperandIndex = {
      {"8C /r", 0},     // MOV Sreg to r/m16; MOV Sreg to r/m64
      {"0F 00 /0", 0},  // SLDT r/m16; SLDT r64/m16
      {"0F 00 /1", 0},  // STR r/m16; STR r64/m16
  };
  std::vector<InstructionProto> instructions_to_add;

  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    int operand_index = -1;
    if (gtl::FindCopy(kOperandIndex, instruction.raw_encoding_specification(),
                      &operand_index)) {
      const InstructionFormat& vendor_syntax =
          GetVendorSyntaxWithMostOperandsOrDie(instruction);
      if (operand_index >= vendor_syntax.operands_size()) {
        return InvalidArgumentError(
            absl::StrCat("Unexpected number of operands of instruction: ",
                         instruction.raw_encoding_specification()));
      }
      // We can't rely just on the information in value_size_bits, because
      // technically, even the 32 or 64-bit versions of the instruction often
      // use a 16-bit value, and just leave the other bits undefined (or
      // zeroed). Instead, we need to look at the string representation of the
      // type of the operand.
      if (k16BitOperands.contains(
              vendor_syntax.operands(operand_index).name())) {
        instructions_to_add.push_back(instruction);
      }
    }
  }

  for (InstructionProto& instruction : instructions_to_add) {
    AddOperandSizeOverrideToInstructionProto(&instruction);
    *instruction_set->add_instructions() = instruction;
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    AddOperandSizeOverrideVersionForSpecialCaseInstructions, 3000);

}  // namespace x86
}  // namespace exegesis
