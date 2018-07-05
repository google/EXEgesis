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

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddEvexBInterpretationTest, LegacyAndVexEncoding) {
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
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    })proto";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, Broadcast) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'xmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m128/m64bcst'
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_128_BIT
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'xmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m128/m64bcst'
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_128_BIT
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
        }
      }
    })proto";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, RoundingControl) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDSS'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'xmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m32'
          tags { name: 'er' }
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_IS_IGNORED
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDSS'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'xmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m32'
          tags { name: 'er' }
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_IS_IGNORED
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
        }
      }
    })proto";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, SuppressAllExceptions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VCMPSD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'k1'
          tags { name: 'k2' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m64'
          tags { name: 'sae' }
          usage: USAGE_READ
        }
        operands {
          encoding: IMMEDIATE_VALUE_ENCODING
          name: 'imm8'
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_IS_IGNORED
          vex_w_usage: VEX_W_IS_ONE
        }
        immediate_value_bytes: 1
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VCMPSD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'k1'
          tags { name: 'k2' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'xmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          name: 'xmm3/m64'
          tags { name: 'sae' }
          usage: USAGE_READ
        }
        operands {
          encoding: IMMEDIATE_VALUE_ENCODING
          name: 'imm8'
          usage: USAGE_READ
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_IS_IGNORED
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS
        }
        immediate_value_bytes: 1
      }
    })proto";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, Combined) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'zmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'zmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'zmm3/m512/m64bcst'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_512_BIT
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'zmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'zmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'zmm3/m512/m64bcst'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_512_BIT
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
        }
      }
    })proto";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexOpmaskUsageTest, Combined) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      llvm_mnemonic: 'VCVTSD2SIrr'
      vendor_syntax {
        mnemonic: 'VCVTSD2SI'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 32
          encoding: MODRM_REG_ENCODING
          usage: USAGE_WRITE
          name: 'r32'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 64
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'xmm1'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
        }
      }
    }
    instructions {
      llvm_mnemonic: 'VGATHERDPDYrm'
      vendor_syntax {
        mnemonic: 'VGATHERDPD'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 256
          encoding: MODRM_REG_ENCODING
          usage: USAGE_READ_WRITE
          name: 'ymm1'
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          usage: USAGE_READ
          encoding: VSIB_ENCODING
          name: 'vm32x'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 256
          usage: USAGE_READ_WRITE
          name: 'ymm2'
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_256_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          vsib_usage: VSIB_USED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VGATHERDPD'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 128
          encoding: MODRM_REG_ENCODING
          usage: USAGE_WRITE
          name: 'xmm1'
          tags { name: 'k1' }
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          usage: USAGE_READ
          encoding: VSIB_ENCODING
          name: 'vm32x'
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          vsib_usage: VSIB_USED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'zmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'zmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'zmm3/m512/m64bcst'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_512_BIT
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      llvm_mnemonic: 'VCVTSD2SIrr'
      vendor_syntax {
        mnemonic: 'VCVTSD2SI'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 32
          encoding: MODRM_REG_ENCODING
          usage: USAGE_WRITE
          name: 'r32'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 64
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'xmm1'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
        }
      }
    }
    instructions {
      llvm_mnemonic: 'VGATHERDPDYrm'
      vendor_syntax {
        mnemonic: 'VGATHERDPD'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 256
          encoding: MODRM_REG_ENCODING
          usage: USAGE_READ_WRITE
          name: 'ymm1'
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          usage: USAGE_READ
          encoding: VSIB_ENCODING
          name: 'vm32x'
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 256
          usage: USAGE_READ_WRITE
          name: 'ymm2'
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_256_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          vsib_usage: VSIB_USED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VGATHERDPD'
        operands {
          addressing_mode: DIRECT_ADDRESSING
          value_size_bits: 128
          encoding: MODRM_REG_ENCODING
          usage: USAGE_WRITE
          name: 'xmm1'
          tags { name: 'k1' }
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          usage: USAGE_READ
          encoding: VSIB_ENCODING
          name: 'vm32x'
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          vsib_usage: VSIB_USED
          opmask_usage: EVEX_OPMASK_IS_REQUIRED
          masking_operation: EVEX_MASKING_MERGING_ONLY
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VADDPD'
        operands {
          encoding: MODRM_REG_ENCODING
          name: 'zmm1'
          tags { name: 'k1' }
          tags { name: 'z' }
          usage: USAGE_WRITE
        }
        operands { encoding: VEX_V_ENCODING name: 'zmm2' }
        operands {
          encoding: MODRM_RM_ENCODING
          usage: USAGE_READ
          name: 'zmm3/m512/m64bcst'
          tags { name: 'er' }
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: EVEX_PREFIX
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vector_size: VEX_VECTOR_SIZE_512_BIT
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    })proto";
  TestTransform(AddEvexOpmaskUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexPseudoOperandsTest, AddPseudoOperands) {
  static constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 512
          name: "zmm2"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 512
          name: "zmm3"
          tags { name: "er" }
          usage: USAGE_READ
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VCMPPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          name: "k1"
          tags { name: "k2" }
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 512
          name: "zmm2"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 512
          name: "zmm3"
          tags { name: "sae" }
          usage: USAGE_READ
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: "imm8"
          usage: USAGE_READ
        }
      }
    }
  )proto";
  static constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 512
          name: "zmm2"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 512
          name: "zmm3"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: X86_STATIC_PROPERTY_ENCODING
          tags { name: "er" }
          usage: USAGE_READ
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VCMPPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          name: "k1"
          tags { name: "k2" }
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 512
          name: "zmm2"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 512
          name: "zmm3"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: X86_STATIC_PROPERTY_ENCODING
          usage: USAGE_READ
          tags { name: "sae" }
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: "imm8"
          usage: USAGE_READ
        }
      }
    }
  )proto";
  TestTransform(AddEvexPseudoOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
