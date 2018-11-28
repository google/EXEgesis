// Copyright 2018 Google Inc.
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

#include "exegesis/x86/cleanup_instruction_set_merge_instructions.h"

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/status_util.h"
#include "src/google/protobuf/util/message_differencer.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::util::MessageDifferencer;

std::string VendorSyntaxMergeKey(const InstructionProto& instruction) {
  std::string key = instruction.raw_encoding_specification();
  const InstructionFormat& vendor_syntax =
      GetVendorSyntaxWithMostOperandsOrDie(instruction);
  for (const InstructionOperand& operand : vendor_syntax.operands()) {
    // We need to keep separation by addressing modes as the register version
    // and memory version(s) are treated as different instructions in our
    // database, and they have different performance characteristics. The
    // addressing mode of all operands except for that in modrm.rm are
    // determined by the opcode, so we only need to add the addressing mode of
    // the modrm.rm operand to the key.
    if (operand.encoding() == InstructionOperand::MODRM_RM_ENCODING) {
      absl::StrAppend(&key, "\t", operand.addressing_mode());
    }
  }
  return key;
}

}  // namespace

Status MergeVendorSyntax(InstructionSetProto* instruction_set) {
  constexpr int kIgnoredFields[] = {
      InstructionProto::kAttSyntaxFieldNumber,
      InstructionProto::kDescriptionFieldNumber,
      InstructionProto::kEncodingSchemeFieldNumber,
      InstructionProto::kSyntaxFieldNumber,
      InstructionProto::kVendorSyntaxFieldNumber};
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  absl::flat_hash_map<std::string, std::vector<InstructionProto*>>
      instructions_by_encoding;
  for (InstructionProto& instruction : *instructions) {
    instructions_by_encoding[VendorSyntaxMergeKey(instruction)].push_back(
        &instruction);
  }

  // Set up a differencer for checking that the merged instructions are
  // equivalent except for the vendor syntax.
  MessageDifferencer differencer;
  for (const int field_number : kIgnoredFields) {
    const google::protobuf::FieldDescriptor* const field =
        InstructionProto::descriptor()->FindFieldByNumber(field_number);
    CHECK(field != nullptr) << "Field was not found: " << field_number;
    differencer.IgnoreField(field);
  }

  Status status;
  absl::flat_hash_set<const InstructionProto*> instructions_to_remove;
  for (const auto& instructions_elem : instructions_by_encoding) {
    const std::vector<InstructionProto*>& instructions =
        instructions_elem.second;
    CHECK_GT(instructions.size(), 0);
    // We pronounce the first instruction of the group to be the canonical
    // version. All other instructions from the group are merged to this
    // instruction.
    InstructionProto& canonical = *instructions[0];
    for (int i = 1; i < instructions.size(); ++i) {
      const InstructionProto& merged = *instructions[i];
      std::string diff;
      differencer.ReportDifferencesToString(&diff);
      if (!differencer.Compare(canonical, merged)) {
        UpdateStatus(
            &status,
            InvalidArgumentError(absl::StrCat(
                "Merged instructions are not equivalent!\nCanonical:\n",
                canonical.DebugString(), "\nMerged:\n", merged.DebugString(),
                "\nDiff:\n", diff)));
      }
      for (const InstructionFormat& vendor_syntax : merged.vendor_syntax()) {
        *canonical.add_vendor_syntax() = vendor_syntax;
      }
      instructions_to_remove.insert(&merged);
    }
  }

  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [&](const InstructionProto& instruction) {
                       return instructions_to_remove.contains(&instruction);
                     }),
      instructions->end());

  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(MergeVendorSyntax, kNotInDefaultPipeline);

Status RemoveUselessOperandPermutations(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);

  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const InstructionFormat& syntax = instruction.vendor_syntax(0);
    bool synonyms = true;
    for (int i = 1; synonyms && i < instruction.vendor_syntax_size(); ++i) {
      const InstructionFormat& other_syntax = instruction.vendor_syntax(i);
      if (syntax.mnemonic() != other_syntax.mnemonic() ||
          syntax.operands_size() != other_syntax.operands_size()) {
        synonyms = false;
        break;
      }
      for (int operand = 0; synonyms && operand < syntax.operands_size();
           ++operand) {
        const InstructionOperand& operand_a = syntax.operands(operand);
        const InstructionOperand& operand_b = other_syntax.operands(operand);
        if (operand_a.name() != operand_b.name() ||
            operand_a.addressing_mode() != operand_b.addressing_mode()) {
          synonyms = false;
        }
      }
    }
    if (synonyms) {
      // If the syntaxes are synonyms, only keep the first one.
      auto* const vendor_syntax = instruction.mutable_vendor_syntax();
      vendor_syntax->erase(vendor_syntax->begin() + 1, vendor_syntax->end());
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveUselessOperandPermutations,
                                   kNotInDefaultPipeline);

}  // namespace x86
}  // namespace exegesis
