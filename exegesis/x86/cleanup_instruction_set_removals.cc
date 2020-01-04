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

#include "exegesis/x86/cleanup_instruction_set_removals.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/status_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/util/message_differencer.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::absl::c_linear_search;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::util::MessageDifferencer;

std::string MakeKey(absl::string_view group_name,
                    absl::string_view short_desc) {
  return absl::StrCat(group_name, "-", short_desc);
}

}  // namespace

Status RemoveDuplicateInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  absl::flat_hash_set<std::string> visited_instructions;

  // A function that keeps track of instruction it has already encountered.
  // Returns true if an instruction was already seen, false otherwise.
  auto remove_instruction_if_visited =
      [&visited_instructions](const InstructionProto& instruction) {
        const std::string serialized_instruction =
            instruction.SerializeAsString();
        return !visited_instructions.insert(serialized_instruction).second;
      };

  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     remove_instruction_if_visited),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveDuplicateInstructions, 4000);

Status RemoveEmptyInstructionGroups(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);

  // Map of instruction group name+short_description to the group proto and a
  // vector of pointers to the instructions in that group.
  //
  // We have to use both name and short_description because there are some
  // groups with the same name, eg
  // "MOV—Move" vs "MOV—Move to/from Control Registers".
  std::map<std::string,
           std::pair<InstructionGroupProto, std::vector<InstructionProto*>>>
      group_to_instructions;
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    const auto& group = instruction_set->instruction_groups(
        instruction.instruction_group_index());
    const std::string key = MakeKey(group.name(), group.short_description());
    auto& pair = group_to_instructions[key];
    if (pair.second.empty()) {
      // We only set the group proto once, to avoid too many copying.
      pair.first = group;
    } else {
      // Check that this instruction has the same instruction_group_index as the
      // previous one.
      CHECK_EQ(pair.second.back()->instruction_group_index(),
               instruction.instruction_group_index())
          << "Inconsistent group index for instruction "
          << instruction.feature_name() << ", of group " << key;
    }
    pair.second.push_back(&instruction);
  }

  // Report removed instruction groups.
  for (const auto& group : instruction_set->instruction_groups()) {
    const std::string key = MakeKey(group.name(), group.short_description());
    if (!gtl::ContainsKey(group_to_instructions, key)) {
      LOG(INFO) << "Removed empty instruction group " << key;
    }
  }

  // Delete all groups and only add back groups that have instructions.
  instruction_set->clear_instruction_groups();
  int group_idx = 0;
  for (auto& it : group_to_instructions) {
    *instruction_set->add_instruction_groups() = std::move(it.second.first);
    for (auto* instruction_ptr : it.second.second) {
      instruction_ptr->set_instruction_group_index(group_idx);
    }
    ++group_idx;
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveEmptyInstructionGroups, 8000);

Status RemoveLegacyVersionsOfInstructions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_set<std::string> kEncodingSpecifications = {"C9",
                                                                    "E3 cb"};
  absl::flat_hash_set<std::string> found_legacy_version;
  absl::flat_hash_set<std::string> found_64bit_version;
  RepeatedPtrField<InstructionProto> instructions =
      std::move(*instruction_set->mutable_instructions());
  for (InstructionProto& instruction : instructions) {
    if (!kEncodingSpecifications.contains(
            instruction.raw_encoding_specification())) {
      *instruction_set->add_instructions() = std::move(instruction);
    } else if (!instruction.legacy_instruction()) {
      // The 64-bit version has 'legacy_version' set to false.
      found_64bit_version.insert(instruction.raw_encoding_specification());
      *instruction_set->add_instructions() = std::move(instruction);
    } else {
      // Legacy (16- or 32-bit) version of the instruction.
      found_legacy_version.insert(instruction.raw_encoding_specification());
    }
  }
  for (const std::string& encoding : kEncodingSpecifications) {
    if (found_legacy_version.contains(encoding) &&
        !found_64bit_version.contains(encoding)) {
      return InvalidArgumentError(absl::StrCat(
          "The 64-bit version of the instruction was not found: ", encoding));
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveLegacyVersionsOfInstructions, 0);

Status RemoveInstructionsWaitingForFpuSync(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): The space after the opcode is important, because with it,
  // the prefix does not match the stand-alone FWAIT instructions that is
  // encoded as "9B".
  static constexpr char kFWaitPrefix[] = "9B ";
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_fwait_for_sync = [](const InstructionProto& instruction) {
    return absl::StartsWith(instruction.raw_encoding_specification(),
                            kFWaitPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_fwait_for_sync),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveInstructionsWaitingForFpuSync, 0);

Status RemoveNonEncodableInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [](const InstructionProto& instruction) {
                       return !instruction.available_in_64_bit();
                     }),
      instructions->end());
  return OkStatus();
}
// NOTE(ondrasej): We can remove the non-encodable instructions only after
// fixing up the availability in 64-bits for LAHF and SAHF.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveNonEncodableInstructions, 110);

Status RemoveRepAndRepneInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): We're comparing the REP prefix without the space after it.
  // This will match also the REPE and REPNE prefixes. On the other hand, there
  // are no instructions that would use REP in their mnemonic, so optimizing
  // the matching this way is safe.
  static constexpr char kRepPrefix[] = "REP";
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_rep_or_repne = [](const InstructionProto& instruction) {
    return absl::StartsWith(GetUniqueVendorSyntaxOrDie(instruction).mnemonic(),
                            kRepPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_rep_or_repne),
                      instructions->end());
  return OkStatus();
}
// TODO(ondrasej): In addition to removing them, we should also add an attribute
// saying whether the REP/REPE/REPNE prefix is allowed.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveRepAndRepneInstructions, 0);

const absl::flat_hash_set<std::string>* const kRemovedEncodingSpecifications =
    new absl::flat_hash_set<std::string>(
        {// Specializations of the ENTER instruction that create stack frame
         // pointer. There is a more generic encoding scheme C8 iw ib that
         // already covers both of these cases.
         "C8 iw 00", "C8 iw 01",
         // Specializations of several x87 floating point instructions. These
         // are "operand-less" versions of the instruction that take ST(0) and
         // ST(1) as operands. However, they are just specialization of the more
         // generic encoding scheme that encodes one of the operands in the
         // opcode.
         "DD E1", "DD E9", "DE C1", "DE E1", "DE F1", "DE F9",
         // The prefixes. They are listed as XACQUIRE and XRELEASE instructions
         // by the Intel manual, but they can only exist as a part of a larger
         // instruction, never on their own.
         "F2", "F3",
         // The CR8 version of the MOV instruction that writes to the control
         // registers CR0-CR8. These are just specialized versions of the
         // instruction that writes to CR0-CR7 (they add the REX.R bit, and they
         // replace /r in the specification with /0, because no other value of
         // the modrm.reg bits are allowed).
         "REX.R + 0F 20 /0", "REX.R + 0F 22 /0",
         // A version of CRC32 r32, r/m8 that has the REX prefix specified.
         // There is also another version of this instruction without this
         // prefix.
         // Since the REX prefix does not prescribe any particular bit to be
         // set, we believe that it is there simply to say that the instruction
         // may use it to access extended registers.
         "F2 REX 0F 38 F0 /r"});
// NOTE(ondrasej): XLAT is not recognized by the LLVM assembler (unlike its
// no-operand version XLATB).
const absl::flat_hash_set<std::string>* const kRemovedMnemonics =
    new absl::flat_hash_set<std::string>({"XLAT"});

Status RemoveSpecialCaseInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto is_special_case_instruction =
      [](const InstructionProto& instruction) {
        if (kRemovedEncodingSpecifications->contains(
                instruction.raw_encoding_specification())) {
          return true;
        }
        return ContainsVendorSyntaxMnemonic(*kRemovedMnemonics, instruction);
      };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     is_special_case_instruction),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveSpecialCaseInstructions, 0);

Status RemoveDuplicateInstructionsWithRexPrefix(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): We need to store a copy of the instruction protos, not
  // pointers those in instruction_set, because the contents of instruction_set
  // is modified by std::remove_if().
  absl::node_hash_map<std::string, std::vector<InstructionProto>>
      instructions_by_encoding;
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    const std::string& specification = instruction.raw_encoding_specification();
    instructions_by_encoding[specification].push_back(instruction);
  }
  Status result = OkStatus();
  const auto is_duplicate_instruction_with_rex =
      [&](const InstructionProto& instruction) {
        static constexpr char kRexPrefixRegex[] = R"(REX *\+ *)";
        static const LazyRE2 rex_prefix_parser = {kRexPrefixRegex};
        absl::string_view specification =
            instruction.raw_encoding_specification();
        if (!RE2::Consume(&specification, *rex_prefix_parser)) return false;
        const std::vector<InstructionProto>* const other_instructions =
            gtl::FindOrNull(instructions_by_encoding,
                            std::string(specification));
        // We remove the instruction only if there is a version without the REX
        // prefix that is equivalent in terms of vendor_syntax. If there is not,
        // we return an error status, and keep the instruction to allow
        // debugging with --exegesis_ignore_failing_transforms.
        //
        // Note that there are cases in the manual, where the REX prefix
        // actually means REX.W. Such cases are fixed by
        // FixRexPrefixSpecification() which runs in the default pipeline
        // before this transform, and they should not cause any failures here.
        if (other_instructions == nullptr) {
          const Status error = InvalidArgumentError(absl::StrCat(
              "Instruction does not have a version without the REX prefix: ",
              instruction.raw_encoding_specification()));
          LOG(WARNING) << error;
          UpdateStatus(&result, error);
          return false;
        }
        for (const InstructionProto& other : *other_instructions) {
          if (MessageDifferencer::Equals(
                  GetUniqueVendorSyntaxOrDie(other),
                  GetUniqueVendorSyntaxOrDie(instruction))) {
            return true;
          }
        }
        const Status error = InvalidArgumentError(
            absl::StrCat("The REX and the non-REX versions differ: ",
                         instruction.raw_encoding_specification()));
        LOG(WARNING) << error;
        UpdateStatus(&result, error);
        return false;
      };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     is_duplicate_instruction_with_rex),
                      instructions->end());
  return result;
}
// The checks performed in the cleanup depend on the encoding specification
// fixes done in FixRexPrefixSpecification(), thus it needs to be executed after
// the encoding specification cleanups.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveDuplicateInstructionsWithRexPrefix,
                                   1005);

namespace {

bool InstructionIsMovFromSRegWithOperand(absl::string_view operand_name,
                                         const InstructionProto& instruction) {
  static constexpr char kMovFromSreg64Bit[] = "REX.W + 8C /r";
  static constexpr char kMovFromSreg32Bit[] = "8C /r";
  if (instruction.raw_encoding_specification() != kMovFromSreg32Bit &&
      instruction.raw_encoding_specification() != kMovFromSreg64Bit) {
    return false;
  }
  const InstructionFormat& vendor_syntax =
      GetUniqueVendorSyntaxOrDie(instruction);
  if (vendor_syntax.operands_size() != 2) return false;
  return vendor_syntax.operands(0).name() == operand_name;
}

}  // namespace

Status RemoveDuplicateMovFromSReg(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static constexpr char k32BitOperand[] = "r16/r32/m16";
  static constexpr char k64BitOperand[] = "r64/m16";
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();

  // The two versions of the instruction differ by the first operand:
  // r16/r32/m16 is the "legacy" version with a 16/32-bit register, r64/m16 is
  // the "64-bit" version with a 64-bit register. We remove the former, but we
  // also check that the latter that we keep is present too.
  const bool has_64_bit_version = std::any_of(
      instructions->begin(), instructions->end(),
      [](const InstructionProto& instruction) {
        return InstructionIsMovFromSRegWithOperand(k64BitOperand, instruction);
      });
  const auto new_instructions_end = std::remove_if(
      instructions->begin(), instructions->end(),
      [](const InstructionProto& instruction) {
        return InstructionIsMovFromSRegWithOperand(k32BitOperand, instruction);
      });
  const bool removed_32_bit_version =
      new_instructions_end != instructions->end();
  instructions->erase(new_instructions_end, instructions->end());
  return (removed_32_bit_version && !has_64_bit_version)
             ? InvalidArgumentError(
                   "The 64-bit version of REX.W + 8C /r was not found")
             : OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveDuplicateMovFromSReg, 0);

Status RemoveX87InstructionsWithGeneralVersions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_set<std::string> kRemovedEncodingSpecifications = {
      "D8 D1", "D8 D9", "DE C9", "DE E9", "D9 C9"};
  google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [&kRemovedEncodingSpecifications](
                         const InstructionProto& instruction) {
                       return kRemovedEncodingSpecifications.contains(
                           instruction.raw_encoding_specification());
                     }),
      instructions->end());

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveX87InstructionsWithGeneralVersions, 0);

}  // namespace x86
}  // namespace exegesis
