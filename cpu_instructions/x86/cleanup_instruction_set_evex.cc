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

#include "cpu_instructions/x86/cleanup_instruction_set_evex.h"

#include "cpu_instructions/base/cleanup_instruction_set.h"
#include "cpu_instructions/proto/x86/encoding_specification.pb.h"
#include "glog/logging.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {

using ::cpu_instructions::util::Status;

constexpr auto EVEX_B_IS_NOT_USED =
    VexPrefixEncodingSpecification::EVEX_B_IS_NOT_USED;
constexpr auto EVEX_B_ENABLES_32_BIT_BROADCAST =
    VexPrefixEncodingSpecification::EVEX_B_ENABLES_32_BIT_BROADCAST;
constexpr auto EVEX_B_ENABLES_64_BIT_BROADCAST =
    VexPrefixEncodingSpecification::EVEX_B_ENABLES_64_BIT_BROADCAST;
constexpr auto EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL =
    VexPrefixEncodingSpecification::EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL;
constexpr auto EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS =
    VexPrefixEncodingSpecification::EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS;

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
    if (encoding_specification->vex_prefix().prefix_type() !=
        VexPrefixEncodingSpecification::EVEX_PREFIX) {
      continue;
    }

    // Check for operands that broadcast a single value from a memory location
    // to all slots in a vector register.
    const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      if (operand.name().find(kBroadcast32Bit) != string::npos) {
        vex_prefix->add_evex_b_interpretation(EVEX_B_ENABLES_32_BIT_BROADCAST);
        break;
      } else if (operand.name().find(kBroadcast64Bit) != string::npos) {
        vex_prefix->add_evex_b_interpretation(EVEX_B_ENABLES_64_BIT_BROADCAST);
        break;
      }
    }

    // Check for the static rounding and suppress all exceptions tags on one of
    // the operands.
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      for (const InstructionOperand::Tag& tag : operand.tags()) {
        if (tag.name() == kEmbeddedRounding) {
          vex_prefix->add_evex_b_interpretation(
              EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL);
        } else if (tag.name() == kSuppressAllExceptions) {
          vex_prefix->add_evex_b_interpretation(
              EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS);
          continue;
        }
      }
    }
  }
  return ::util::OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddEvexBInterpretation, 5500);

}  // namespace x86
}  // namespace cpu_instructions
