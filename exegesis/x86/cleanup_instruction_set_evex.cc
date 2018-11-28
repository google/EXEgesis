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

#include "exegesis/x86/cleanup_instruction_set_evex.h"

#include <algorithm>
#include <string>
#include <unordered_set>

#include "absl/container/node_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/category_util.h"
#include "exegesis/util/instruction_syntax.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::google::protobuf::RepeatedPtrField;

Status AddEvexBInterpretation(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kBroadcast32Bit[] = "m32bcst";
  constexpr char kBroadcast64Bit[] = "m64bcst";
  constexpr char kEmbeddedRounding[] = "er";
  constexpr char kSuppressAllExceptions[] = "sae";
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    EncodingSpecification* const encoding_specification =
        instruction.mutable_x86_encoding_specification();
    if (!encoding_specification->has_vex_prefix()) continue;
    VexPrefixEncodingSpecification* const vex_prefix =
        encoding_specification->mutable_vex_prefix();

    // VEX-only instructions can't use the EVEX.b bit.
    if (vex_prefix->prefix_type() != EVEX_PREFIX) continue;

    // Check for operands that broadcast a single value from a memory location
    // to all slots in a vector register.
    const InstructionFormat& vendor_syntax =
        GetVendorSyntaxWithMostOperandsOrDie(instruction);
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      if (absl::StrContains(operand.name(), kBroadcast32Bit)) {
        vex_prefix->add_evex_b_interpretations(EVEX_B_ENABLES_32_BIT_BROADCAST);
        break;
      } else if (absl::StrContains(operand.name(), kBroadcast64Bit)) {
        vex_prefix->add_evex_b_interpretations(EVEX_B_ENABLES_64_BIT_BROADCAST);
        break;
      }
    }

    // Check for the static rounding and suppress all exceptions tags on one of
    // the operands.
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      for (const InstructionOperand::Tag& tag : operand.tags()) {
        if (tag.name() == kEmbeddedRounding) {
          vex_prefix->add_evex_b_interpretations(
              EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL);
        } else if (tag.name() == kSuppressAllExceptions) {
          vex_prefix->add_evex_b_interpretations(
              EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS);
          continue;
        }
      }
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddEvexBInterpretation, 5500);

Status AddEvexOpmaskUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // The list of instructions that do not allow using k0 as opmask. This
  // behavior is specified only in the free-text description of the instruction,
  // so we had to list them explicitly by their mnemonic.
  const absl::node_hash_set<std::string> kOpmaskRequiredMnemonics = {
      "VGATHERDPS",  "VGATHERDPD",  "VGATHERQPS",  "VGATHERQPD",
      "VPGATHERDD",  "VPGATHERDQ",  "VPGATHERQD",  "VPGATHERQQ",
      "VPSCATTERDD", "VPSCATTERDQ", "VPSCATTERQD", "VPSCATTERQQ",
      "VSCATTERDPS", "VSCATTERDPD", "VSCATTERQPS", "VSCATTERQPD"};
  constexpr char kOpmaskRegisterTag[] = "k1";
  constexpr char kOpmaskZeroingTag[] = "z";
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    EncodingSpecification* const encoding_specification =
        instruction.mutable_x86_encoding_specification();
    if (!encoding_specification->has_vex_prefix()) continue;
    VexPrefixEncodingSpecification* const vex_prefix =
        encoding_specification->mutable_vex_prefix();
    vex_prefix->set_masking_operation(NO_EVEX_MASKING);
    vex_prefix->set_opmask_usage(EVEX_OPMASK_IS_NOT_USED);

    // VEX-only instructions can't use opmasks.
    if (vex_prefix->prefix_type() != EVEX_PREFIX) continue;

    const InstructionFormat& vendor_syntax =
        GetVendorSyntaxWithMostOperandsOrDie(instruction);
    bool supports_opmask = false;
    bool supports_zeroing = false;
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      for (const InstructionOperand::Tag& tag : operand.tags()) {
        if (tag.name() == kOpmaskRegisterTag) {
          supports_opmask = true;
        } else if (tag.name() == kOpmaskZeroingTag) {
          supports_zeroing = true;
        }
      }
    }

    if (!supports_opmask) {
      // The instruction does not support opmasks.
      if (supports_zeroing) {
        return InvalidArgumentError(absl::StrCat(
            "Instructopn supports zeroing without also supporting opmasks: ",
            instruction.DebugString()));
      }
      continue;
    }
    const bool requires_opmask =
        gtl::ContainsKey(kOpmaskRequiredMnemonics, vendor_syntax.mnemonic());
    vex_prefix->set_opmask_usage(requires_opmask ? EVEX_OPMASK_IS_REQUIRED
                                                 : EVEX_OPMASK_IS_OPTIONAL);
    vex_prefix->set_masking_operation(supports_zeroing
                                          ? EVEX_MASKING_MERGING_AND_ZEROING
                                          : EVEX_MASKING_MERGING_ONLY);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddEvexOpmaskUsage, 5500);

namespace {

inline bool IsPseudoOperandTag(const InstructionOperand::Tag& tag) {
  static const auto* const kPseudoOperandTags =
      new absl::node_hash_set<std::string>({"er", "sae"});
  return gtl::ContainsKey(*kPseudoOperandTags, tag.name());
}

bool OperandHasPseudoOperandTag(const InstructionOperand& operand) {
  return std::any_of(operand.tags().begin(), operand.tags().end(),
                     IsPseudoOperandTag);
}

bool InstructionHasPseudoOperandTag(const InstructionFormat& syntax) {
  return std::any_of(syntax.operands().begin(), syntax.operands().end(),
                     OperandHasPseudoOperandTag);
}

bool IsPseudoOperand(const InstructionOperand& operand) {
  return operand.name().empty() &&
         operand.addressing_mode() == InstructionOperand::NO_ADDRESSING &&
         InCategory(operand.encoding(),
                    InstructionOperand::X86_STATIC_PROPERTY_ENCODING);
}

void RemovePseudoOperandTags(InstructionOperand* operand) {
  CHECK(operand != nullptr);
  RepeatedPtrField<InstructionOperand::Tag>* const tags =
      operand->mutable_tags();
  tags->erase(std::remove_if(tags->begin(), tags->end(),
                             [](const InstructionOperand::Tag& tag) {
                               return IsPseudoOperandTag(tag);
                             }),
              tags->end());
}

}  // namespace

Status AddEvexPseudoOperands(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    InstructionFormat* const vendor_syntax =
        GetOrAddUniqueVendorSyntaxOrDie(&instruction);
    if (!InstructionHasPseudoOperandTag(*vendor_syntax)) continue;

    RepeatedPtrField<InstructionOperand> updated_operands;
    bool instruction_has_pseudo_operand = false;
    for (const InstructionOperand& operand : vendor_syntax->operands()) {
      InstructionOperand* const updated_operand = updated_operands.Add();
      *updated_operand = operand;
      if (IsPseudoOperand(operand)) {
        CHECK(!instruction_has_pseudo_operand) << instruction.DebugString();
        instruction_has_pseudo_operand = true;
        continue;
      }
      if (!OperandHasPseudoOperandTag(operand)) continue;
      CHECK(!instruction_has_pseudo_operand) << instruction.DebugString();
      instruction_has_pseudo_operand = true;

      RemovePseudoOperandTags(updated_operand);
      InstructionOperand* const pseudo_operand = updated_operands.Add();
      pseudo_operand->set_addressing_mode(InstructionOperand::NO_ADDRESSING);
      pseudo_operand->set_encoding(
          InstructionOperand::X86_STATIC_PROPERTY_ENCODING);
      pseudo_operand->set_usage(InstructionOperand::USAGE_READ);
      for (const InstructionOperand::Tag& tag : operand.tags()) {
        if (IsPseudoOperandTag(tag)) {
          *pseudo_operand->add_tags() = tag;
        }
      }
    }
    vendor_syntax->mutable_operands()->Swap(&updated_operands);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddEvexPseudoOperands, 5500);

}  // namespace x86
}  // namespace exegesis
