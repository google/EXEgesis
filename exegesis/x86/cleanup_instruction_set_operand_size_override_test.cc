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

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddOperandSizeOverrideToInstructionsWithImplicitOperandsTest, AddPrefix) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'STOSQ' }
      raw_encoding_specification: 'REX.W + AB'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'STOSW' }
      raw_encoding_specification: 'AB'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r16'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '58+ rw'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        immediate_value_bytes: 2
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'STOSQ' }
      raw_encoding_specification: 'REX.W + AB'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'STOSW' }
      raw_encoding_specification: '66 AB'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r16'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '58+ rw'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        immediate_value_bytes: 2
      }
    })proto";
  TestTransform(AddOperandSizeOverrideToInstructionsWithImplicitOperands,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverrideVersionForSpecialCaseInstructions,
     AddInstructionWithPrefix) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r/m16" }
        operands { name: "Sreg" }
      }
      raw_encoding_specification: "8C /r"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SLDT"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "0F 00 /0"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "STR"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "0F 00 /1"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SLDT"
        operands { name: "r64/m16" }
      }
      raw_encoding_specification: "0F 00 /0"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r/m64" }
        operands { name: "Sreg" }
      }
      raw_encoding_specification: "8C /r"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SAR"
        operands { name: "r/m32" }
        operands { name: "cl" }
      }
      raw_encoding_specification: "D3 /7"
      x86_encoding_specification { legacy_prefixes {} }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r/m16" }
        operands { name: "Sreg" }
      }
      raw_encoding_specification: "8C /r"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SLDT"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "0F 00 /0"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "STR"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "0F 00 /1"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SLDT"
        operands { name: "r64/m16" }
      }
      raw_encoding_specification: "0F 00 /0"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r/m64" }
        operands { name: "Sreg" }
      }
      raw_encoding_specification: "8C /r"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SAR"
        operands { name: "r/m32" }
        operands { name: "cl" }
      }
      raw_encoding_specification: "D3 /7"
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands { name: "r/m16" }
        operands { name: "Sreg" }
      }
      raw_encoding_specification: "66 8C /r"
      x86_encoding_specification {
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "SLDT"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "66 0F 00 /0"
      x86_encoding_specification {
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "STR"
        operands { name: "r/m16" }
      }
      raw_encoding_specification: "66 0F 00 /1"
      x86_encoding_specification {
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
      }
    })proto";
  TestTransform(AddOperandSizeOverrideVersionForSpecialCaseInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverrideToSpecialCaseInstructionsTest, AddPrefix) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r32/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r64/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'REX.W + 0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r16'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '58+ rw'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 64
        }
      }
      raw_encoding_specification: '58+ rd'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CRC32'
        operands {
          name: 'r32'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'F2 0F 38 F1 /r'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          has_mandatory_repne_prefix: true
        }
        modrm_usage: FULL_MODRM
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CRC32'
        operands {
          name: 'r32'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'F2 0F 38 F1 /r'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          has_mandatory_repne_prefix: true
        }
        modrm_usage: FULL_MODRM
      }
    }
  )proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r32/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SMSW'
        operands {
          name: 'r64/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'REX.W + 0F 01 /4'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r16'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 58+ rw'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 64
        }
      }
      raw_encoding_specification: '58+ rd'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CRC32'
        operands {
          name: 'r32'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 F2 0F 38 F1 /r'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          has_mandatory_repne_prefix: true
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: FULL_MODRM
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CRC32'
        operands {
          name: 'r32'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'F2 0F 38 F1 /r'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          has_mandatory_repne_prefix: true
        }
        modrm_usage: FULL_MODRM
      }
    })proto";
  TestTransform(AddOperandSizeOverrideToSpecialCaseInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverridePrefixTest, AddPrefix) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm16'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '81 /2 iw'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: '81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m64'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'REX.W + 81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'AX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'EF'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'EAX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'EF'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm16'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 81 /2 iw'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: '81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '66 83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m64'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'REX.W + 81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'AX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 EF'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'EAX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'EF'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    })proto";
  TestTransform(AddOperandSizeOverridePrefix, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverridePrefixUsageTest, AddUsage) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm16'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 81 /2 iw'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: '81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '66 83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m64'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'REX.W + 81 /2 id'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'AX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 EF'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'EAX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'EF'
      x86_encoding_specification {
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "CPUID" }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "0F A2"
      x86_encoding_specification {
        opcode: 4002
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm16'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 81 /2 iw'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: '81 /2 id'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m16'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '66 83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m32'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
        operands {
          name: 'imm8'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
        }
      }
      raw_encoding_specification: '83 /2 ib'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'ADC'
        operands {
          name: 'r/m64'
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
        }
        operands {
          name: 'imm32'
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'REX.W + 81 /2 id'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'AX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 EF'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUT'
        operands {
          name: 'DX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
        operands {
          name: 'EAX'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 32
        }
      }
      raw_encoding_specification: 'EF'
      x86_encoding_specification {
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "CPUID" }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "0F A2"
      x86_encoding_specification {
        opcode: 4002
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    })proto";
  TestTransform(AddOperandSizeOverridePrefixUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
