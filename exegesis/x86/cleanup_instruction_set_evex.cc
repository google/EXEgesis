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

#include <unordered_set>
#include "strings/string.h"

#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::Status;
using ::exegesis::util::OkStatus;

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
    const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      if (operand.name().find(kBroadcast32Bit) != string::npos) {
        vex_prefix->add_evex_b_interpretations(EVEX_B_ENABLES_32_BIT_BROADCAST);
        break;
      } else if (operand.name().find(kBroadcast64Bit) != string::npos) {
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
  const std::unordered_set<string> kOpmaskRequiredMnemonics = {
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

    const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
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
        return InvalidArgumentError(StrCat(
            "Instructopn supports zeroing without also supporting opmasks: ",
            instruction.DebugString()));
      }
      continue;
    }
    const bool requires_opmask =
        ContainsKey(kOpmaskRequiredMnemonics, vendor_syntax.mnemonic());
    vex_prefix->set_opmask_usage(requires_opmask ? EVEX_OPMASK_IS_REQUIRED
                                                 : EVEX_OPMASK_IS_OPTIONAL);
    vex_prefix->set_masking_operation(supports_zeroing
                                          ? EVEX_MASKING_MERGING_AND_ZEROING
                                          : EVEX_MASKING_MERGING_ONLY);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddEvexOpmaskUsage, 5500);

}  // namespace x86
}  // namespace exegesis
