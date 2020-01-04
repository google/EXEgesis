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

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::HasSubstr;

TEST(RemoveDuplicateInstructionsTest, RemoveThem) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
        operands { name: 'xmm3/m128' }
      }
      raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'ymm1' }
        operands { name: 'ymm2' }
        operands { name: 'ymm3/m256' }
      }
      raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'ymm1' }
        operands { name: 'ymm2' }
        operands { name: 'ymm3/m256' }
      }
      raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'ymm1' }
        operands { name: 'ymm2' }
        operands { name: 'ymm3/m256' }
      }
      raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
        operands { name: 'xmm3/m128' }
      }
      raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'ymm1' }
        operands { name: 'ymm2' }
        operands { name: 'ymm3/m256' }
      }
      raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r'
    })proto";
  TestTransform(RemoveDuplicateInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveDuplicateInstructionsTest, NoRemoval) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'ymm1' }
        operands { name: 'ymm2' }
        operands { name: 'ymm3/m256' }
      }
      raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VPCMPEQQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
        operands { name: 'xmm3/m128' }
      }
      raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r'
    })proto";
  TestTransform(RemoveDuplicateInstructions, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(RemoveEmptyInstructionGroupsTest, RemoveAndResortGroups) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAA' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'AAD' }
      instruction_group_index: 3
    }
    instructions {
      vendor_syntax { mnemonic: 'AAM' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'ADD' }
      instruction_group_index: 0
    }
    instruction_groups { name: 'GROUP_D' description: 'Non-empty, should be 2' }
    instruction_groups { name: 'GROUP_C' description: 'No instructions' }
    instruction_groups { name: 'GROUP_B' description: 'Non-empty, should be 1' }
    instruction_groups { name: 'GROUP_A' description: 'Non-empty, should be 0' }
  )proto";

  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAA' }
      instruction_group_index: 1
    }
    instructions {
      vendor_syntax { mnemonic: 'AAD' }
      instruction_group_index: 0
    }
    instructions {
      vendor_syntax { mnemonic: 'AAM' }
      instruction_group_index: 1
    }
    instructions {
      vendor_syntax { mnemonic: 'ADD' }
      instruction_group_index: 2
    }
    instruction_groups { name: 'GROUP_A' description: 'Non-empty, should be 0' }
    instruction_groups { name: 'GROUP_B' description: 'Non-empty, should be 1' }
    instruction_groups { name: 'GROUP_D' description: 'Non-empty, should be 2' }
  )proto";

  TestTransform(RemoveEmptyInstructionGroups, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveEmptyInstructionGroupsTest, RemoveAndResortGroupsSameName) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAA' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'AAD' }
      instruction_group_index: 3
    }
    instructions {
      vendor_syntax { mnemonic: 'AAM' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'ADD' }
      instruction_group_index: 0
    }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 2'
    }
    instruction_groups { name: 'GROUP' short_description: 'No instructions' }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 1'
    }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 0'
    }
  )proto";

  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAA' }
      instruction_group_index: 1
    }
    instructions {
      vendor_syntax { mnemonic: 'AAD' }
      instruction_group_index: 0
    }
    instructions {
      vendor_syntax { mnemonic: 'AAM' }
      instruction_group_index: 1
    }
    instructions {
      vendor_syntax { mnemonic: 'ADD' }
      instruction_group_index: 2
    }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 0'
    }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 1'
    }
    instruction_groups {
      name: 'GROUP'
      short_description: 'Non-empty, should be 2'
    }
  )proto";

  TestTransform(RemoveEmptyInstructionGroups, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveEmptyInstructionGroupsTest, NoRemoval) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAA' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'AAD' }
      instruction_group_index: 1
    }
    instructions {
      vendor_syntax { mnemonic: 'AAM' }
      instruction_group_index: 2
    }
    instructions {
      vendor_syntax { mnemonic: 'ADD' }
      instruction_group_index: 0
    }
    instruction_groups { name: 'GROUP_0' description: 'Has some instructions' }
    instruction_groups { name: 'GROUP_1' description: 'Has some instructions' }
    instruction_groups { name: 'GROUP_2' description: 'Has some instructions' }
  )proto";

  TestTransform(RemoveEmptyInstructionGroups, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(RemoveLegacyVersionsOfInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "LEAVE" }
      syntax { mnemonic: "leave" }
      att_syntax { mnemonic: "leave" }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "C9"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 201
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "LEAVE" }
      syntax { mnemonic: "leave" }
      att_syntax { mnemonic: "leave" }
      available_in_64_bit: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "C9"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 201
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
  )proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "LEAVE" }
      syntax { mnemonic: "leave" }
      att_syntax { mnemonic: "leave" }
      available_in_64_bit: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "C9"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 201
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
  )proto";
  TestTransform(RemoveLegacyVersionsOfInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveInstructionsWaitingForFpuSyncTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'FCHS' }
      raw_encoding_specification: 'D9 E0'
    }
    instructions {
      vendor_syntax { mnemonic: 'FCLEX' }
      raw_encoding_specification: '9B DB E2'
    }
    instructions {
      vendor_syntax { mnemonic: 'FWAIT' }
      raw_encoding_specification: '9B'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FNSAVE'
        operands { name: 'm108byte' }
      }
      raw_encoding_specification: 'DD /6'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'FCHS' }
      raw_encoding_specification: 'D9 E0'
    }
    instructions {
      vendor_syntax { mnemonic: 'FWAIT' }
      raw_encoding_specification: '9B'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FNSAVE'
        operands { name: 'm108byte' }
      }
      raw_encoding_specification: 'DD /6'
    })proto";
  TestTransform(RemoveInstructionsWaitingForFpuSync, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveRepAndRepneInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'REP STOS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'F3 AA'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AE'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'REPNE CMPS'
        operands { name: 'm8' }
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'F2 A6'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AA'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'REPE SCAS'
        operands { name: 'm8' }
      }
      legacy_instruction: false
      raw_encoding_specification: 'F3 REX.W AE'
    }
    instructions {
      vendor_syntax { mnemonic: 'CMPSB' }
      raw_encoding_specification: 'A6'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AE'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AA'
    }
    instructions {
      vendor_syntax { mnemonic: 'CMPSB' }
      raw_encoding_specification: 'A6'
    })proto";
  TestTransform(RemoveRepAndRepneInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveNonEncodableInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'AAS' }
      available_in_64_bit: false
      raw_encoding_specification: '3F'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CALL'
        operands { name: 'm16:32' }
      }
      available_in_64_bit: true
      raw_encoding_specification: 'FF /3'
    }
    instructions {
      description: 'Clears TS flag in CR0.'
      vendor_syntax { mnemonic: 'CLTS' }
      available_in_64_bit: true
      raw_encoding_specification: '0F 06'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'DEC'
        operands { name: 'r16' }
      }
      available_in_64_bit: false
      raw_encoding_specification: '66 48+rw'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'CALL'
        operands { name: 'm16:32' }
      }
      raw_encoding_specification: 'FF /3'
      available_in_64_bit: true
    }
    instructions {
      description: 'Clears TS flag in CR0.'
      vendor_syntax { mnemonic: 'CLTS' }
      raw_encoding_specification: '0F 06'
      available_in_64_bit: true
    })proto";
  TestTransform(RemoveNonEncodableInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveSpecialCaseInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FUCOM'
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DD E0+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FUCOM' }
      raw_encoding_specification: 'DD E1'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FUCOMI'
        operands { name: 'ST' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DB E8+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FDIVRP' }
      raw_encoding_specification: 'DE F1'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FFREE'
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DD C0+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FADDP' }
      raw_encoding_specification: 'DE C1'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FUCOM'
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DD E0+i'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FUCOMI'
        operands { name: 'ST' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DB E8+i'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FFREE'
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DD C0+i'
    })proto";
  TestTransform(RemoveSpecialCaseInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveDuplicateInstructionsWithRexPrefixTest, RemoveSomeInstructions) {
  const char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "r64" }
        operands { name: "imm32" }
      }
      raw_encoding_specification: "REX.W + 81 /2 id"
      instruction_group_index: 4
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "80 /2 ib"
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "REX + 80 /2 ib"
      instruction_group_index: 4
    })proto";
  const char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "r64" }
        operands { name: "imm32" }
      }
      raw_encoding_specification: "REX.W + 81 /2 id"
      instruction_group_index: 4
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "80 /2 ib"
    })proto";
  TestTransform(RemoveDuplicateInstructionsWithRexPrefix, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveDuplicateInstructionsWithRexPrefixTest, FailsIfNotDuplicate) {
  constexpr const char* kInstructionSets[] = {
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "LSS"
            operands { name: "r32" }
            operands { name: "m16:32" }
          }
          raw_encoding_specification: "0F B2 /r"
          instruction_group_index: 197
        }
        instructions {
          vendor_syntax {
            mnemonic: "LSS"
            operands { name: "r64" }
            operands { name: "m16:64" }
          }
          raw_encoding_specification: "REX + 0F B2 /r"
          instruction_group_index: 197
        })proto",
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "ADC"
            operands { name: "m8" }
            operands { name: "imm8" }
          }
          raw_encoding_specification: "REX + 80 /2 ib"
          instruction_group_index: 4
        })proto"};
  for (const char* const instruction_set_source : kInstructionSets) {
    SCOPED_TRACE(instruction_set_source);
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(instruction_set_source);
    EXPECT_THAT(RemoveDuplicateInstructionsWithRexPrefix(&instruction_set),
                StatusIs(INVALID_ARGUMENT));
  }
}

TEST(RemoveDuplicateMovFromSRegTest, Remove) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "REX + 80 /2 ib"
      instruction_group_index: 4
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r16/r32/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "66 8C /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r64/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "REX.W + 8C /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r16/r32/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "8C /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r16/r32/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "REX.W + 8C /r"
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "REX + 80 /2 ib"
      instruction_group_index: 4
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r16/r32/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "66 8C /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r64/m16" }
        operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
      }
      raw_encoding_specification: "REX.W + 8C /r"
    })proto";
  TestTransform(RemoveDuplicateMovFromSReg, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveDuplicateMovFromSRegTest, FailsWhenNotDuplicate) {
  InstructionSetProto instruction_set =
      ParseProtoFromStringOrDie<InstructionSetProto>(R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "ADC"
            operands { name: "m8" }
            operands { name: "imm8" }
          }
          raw_encoding_specification: "REX + 80 /2 ib"
          instruction_group_index: 4
        }
        instructions {
          vendor_syntax {
            mnemonic: "MOV"
            operands { name: "r16/r32/m16" }
            operands { name: "Sreg" register_class: SPECIAL_REGISTER_SEGMENT }
          }
          raw_encoding_specification: "REX.W + 8C /r"
        })proto");
  EXPECT_THAT(RemoveDuplicateMovFromSReg(&instruction_set),
              StatusIs(INVALID_ARGUMENT, HasSubstr("was not found")));
}

TEST(RemoveX87InstructionsWithGeneralVersionsTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions { raw_encoding_specification: "D8 D0+i" }
    instructions { raw_encoding_specification: "D8 D1" }
    instructions { raw_encoding_specification: "D8 /2" })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions { raw_encoding_specification: "D8 D0+i" }
    instructions { raw_encoding_specification: "D8 /2" })proto";
  TestTransform(RemoveX87InstructionsWithGeneralVersions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
