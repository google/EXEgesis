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

#include "exegesis/x86/cleanup_instruction_set_alternatives.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddAlternativesTest, InstructionWithRM8) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 8
          name: 'r8'
        }
        operands {
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: 'r/m8'
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 8
          name: 'r8'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: 'r8'
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 8
          name: 'r8'
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: 'm8'
        }
      }
    })proto";
  TestTransform(AddAlternatives, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddAlternativesTest, DifferentSizes) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'PINSRB'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: 'xmm1'
        }
        operands {
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: 'r32/m8'
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: 'imm8'
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'PINSRB'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: 'xmm1'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: 'r32'
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: 'imm8'
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PINSRB'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: 'xmm1'
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: 'm8'
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: 'imm8'
        }
      }
    })proto";
  TestTransform(AddAlternatives, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddAlternativesTest, NoRenaming) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FBLD'
        operands { name: 'm80bcd' }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FBLD'
        operands { name: 'm80bcd' }
      }
    })proto";
  TestTransform(AddAlternatives, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
