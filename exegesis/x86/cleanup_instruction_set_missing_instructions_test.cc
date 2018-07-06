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

#include "exegesis/x86/cleanup_instruction_set_missing_instructions.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddMissingFfreepInstructionTest, AddFfreep) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AL" usage: USAGE_WRITE }
        operands { name: "imm8" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "I"
      raw_encoding_specification: "14 ib"
      instruction_group_index: 0
    }
    instruction_groups {
      name: "ADC"
      description: "Add with Carry."
      short_description: "Add with Carry."
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AL" usage: USAGE_WRITE }
        operands { name: "imm8" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "I"
      raw_encoding_specification: "14 ib"
      instruction_group_index: 0
    }
    instructions {
      description: "Free Floating-Point Register and Pop."
      vendor_syntax {
        mnemonic: "FFREEP"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          name: "ST(i)"
          usage: USAGE_WRITE
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "M"
      raw_encoding_specification: "DF /0"
      instruction_group_index: 1
    }
    instruction_groups {
      name: "ADC"
      description: "Add with Carry."
      short_description: "Add with Carry."
    }
    instruction_groups {
      name: "FFREEP"
      description: "Free Floating-Point Register and Pop."
      flags_affected {}
      short_description: "Free Floating-Point Register and Pop."
    })proto";
  TestTransform(AddMissingFfreepInstruction, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
