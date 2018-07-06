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

#include "exegesis/x86/cleanup_instruction_set_rex.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddRexWPrefixUsageTest, AddUsage) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "CPUID" }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "0F A2"
      x86_encoding_specification {
        opcode: 0x0FA2
        legacy_prefixes {}
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AX" usage: USAGE_WRITE }
        operands { name: "imm16" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "66 15 iw"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "EAX" usage: USAGE_WRITE }
        operands { name: "imm32" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "15 id"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes {}
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "RAX" usage: USAGE_WRITE }
        operands { name: "imm32" usage: USAGE_READ }
      }
      available_in_64_bit: true
      raw_encoding_specification: "REX.W + 15 id"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ANDN"
        operands { name: "r32a" usage: USAGE_WRITE }
        operands { name: "r32b" usage: USAGE_READ }
        operands { name: "m32" usage: USAGE_READ }
      }
      feature_name: "BMI1"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.NDS.LZ.0F38.W0 F2 /r"
      x86_encoding_specification {
        opcode: 0x0F38F2
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "CALL"
        operands { name: "m64" usage: USAGE_READ }
      }
      available_in_64_bit: true
      raw_encoding_specification: "FF /2"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        legacy_prefixes {}
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m16" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "66 FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m32" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {}
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m64" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      raw_encoding_specification: "REX.W + FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "CPUID" }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "0F A2"
      x86_encoding_specification {
        opcode: 0x0FA2
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AX" usage: USAGE_WRITE }
        operands { name: "imm16" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "66 15 iw"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes {
          operand_size_override_prefix: PREFIX_IS_REQUIRED
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "EAX" usage: USAGE_WRITE }
        operands { name: "imm32" usage: USAGE_READ }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "15 id"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "RAX" usage: USAGE_WRITE }
        operands { name: "imm32" usage: USAGE_READ }
      }
      available_in_64_bit: true
      raw_encoding_specification: "REX.W + 15 id"
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ANDN"
        operands { name: "r32a" usage: USAGE_WRITE }
        operands { name: "r32b" usage: USAGE_READ }
        operands { name: "m32" usage: USAGE_READ }
      }
      feature_name: "BMI1"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.NDS.LZ.0F38.W0 F2 /r"
      x86_encoding_specification {
        opcode: 0x0F38F2
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "CALL"
        operands { name: "m64" usage: USAGE_READ }
      }
      available_in_64_bit: true
      raw_encoding_specification: "FF /2"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m16" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "66 FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          operand_size_override_prefix: PREFIX_IS_REQUIRED
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m32" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes { rex_w_prefix: PREFIX_IS_NOT_PERMITTED }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INC"
        operands { name: "m64" usage: USAGE_READ_WRITE }
      }
      available_in_64_bit: true
      raw_encoding_specification: "REX.W + FF /0"
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes { rex_w_prefix: PREFIX_IS_REQUIRED }
      }
    })proto";
  TestTransform(AddRexWPrefixUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
