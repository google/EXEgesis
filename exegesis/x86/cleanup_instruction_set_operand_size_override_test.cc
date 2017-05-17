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
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'STOSQ' }
           legacy_instruction: false
           encoding_scheme: 'NA'
           raw_encoding_specification: 'REX.W + AB'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true }
             opcode: 0xAB }}
         instructions {
           vendor_syntax { mnemonic: 'STOSW' }
           encoding_scheme: 'NA'
           raw_encoding_specification: 'AB'
           x86_encoding_specification { legacy_prefixes {} opcode: 0xAB }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           raw_encoding_specification: '58+ rw'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x58
             immediate_value_bytes: 2 }})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'STOSQ' }
           legacy_instruction: false
           encoding_scheme: 'NA'
           raw_encoding_specification: 'REX.W + AB'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true }
             opcode: 0xAB }}
         instructions {
           vendor_syntax { mnemonic: 'STOSW' }
           encoding_scheme: 'NA'
           raw_encoding_specification: '66 AB'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0xAB }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           raw_encoding_specification: '58+ rw'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x58
             immediate_value_bytes: 2 }})";
  TestTransform(AddOperandSizeOverrideToInstructionsWithImplicitOperands,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverrideToSpecialCaseInstructionsTest, AddPrefix) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: '0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r32/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: '0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r64/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: 'REX.W + 0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true } opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           raw_encoding_specification: '58+ rw'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x58 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r64' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 64 }}
           legacy_instruction: false
           encoding_scheme: 'O'
           raw_encoding_specification: '58+ rd'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true }
             opcode: 0x58 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'RM'
           raw_encoding_specification: 'F2 0F 38 F1 /r'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_repne_prefix: true }
             opcode: 0x0F38F1 modrm_usage: FULL_MODRM }}
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'RM'
           raw_encoding_specification: 'F2 0F 38 F1 /r'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_repne_prefix: true }
             opcode: 0x0F38F1 modrm_usage: FULL_MODRM }})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: '66 0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r32/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: '0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r64/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           raw_encoding_specification: 'REX.W + 0F 01 /4'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true } opcode: 0x0F01
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           raw_encoding_specification: '66 58+ rw'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0x58 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r64' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 64 }}
           legacy_instruction: false
           encoding_scheme: 'O'
           raw_encoding_specification: '58+ rd'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true }
             opcode: 0x58 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'RM'
           raw_encoding_specification: '66 F2 0F 38 F1 /r'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_repne_prefix: true
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0x0F38F1 modrm_usage: FULL_MODRM }}
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'RM'
           raw_encoding_specification: 'F2 0F 38 F1 /r'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_repne_prefix: true }
             opcode: 0x0F38F1 modrm_usage: FULL_MODRM }})";
  TestTransform(AddOperandSizeOverrideToSpecialCaseInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverridePrefixTest, AddPrefix) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm16' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '81 /2 iw'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '81 /2 id'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '83 /2 ib'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x83
             modrm_usage: OPCODE_EXTENSION_IN_MODRM modrm_opcode_extension: 2
             immediate_value_bytes: 1 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '83 /2 ib'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x83
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 1 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m64'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 64 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           legacy_instruction: false
           encoding_scheme: 'MI'
           raw_encoding_specification: 'REX.W + 81 /2 id'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true } opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM modrm_opcode_extension: 2
             immediate_value_bytes: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'OUT'
             operands { name: 'DX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }
             operands { name: 'AX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'EF'
           x86_encoding_specification { legacy_prefixes {} opcode: 0xEF }}
         instructions {
           vendor_syntax {
             mnemonic: 'OUT'
             operands { name: 'DX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }
             operands { name: 'EAX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'EF'
           x86_encoding_specification { legacy_prefixes {} opcode: 0xEF }})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm16' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '66 81 /2 iw'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 2 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '81 /2 id'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '66 83 /2 ib'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix: true }
             opcode: 0x83
             modrm_usage: OPCODE_EXTENSION_IN_MODRM modrm_opcode_extension: 2
             immediate_value_bytes: 1 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           raw_encoding_specification: '83 /2 ib'
           x86_encoding_specification {
             legacy_prefixes {} opcode: 0x83
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 2 immediate_value_bytes: 1 }}
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m64'
                        addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 64 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           legacy_instruction: false
           encoding_scheme: 'MI'
           raw_encoding_specification: 'REX.W + 81 /2 id'
           x86_encoding_specification {
             legacy_prefixes { has_mandatory_rex_w_prefix: true } opcode: 0x81
             modrm_usage: OPCODE_EXTENSION_IN_MODRM modrm_opcode_extension: 2
             immediate_value_bytes: 4 }}
         instructions {
           vendor_syntax {
             mnemonic: 'OUT'
             operands { name: 'DX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }
             operands { name: 'AX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           raw_encoding_specification: '66 EF'
           x86_encoding_specification {
             legacy_prefixes {
               has_mandatory_operand_size_override_prefix : true }
             opcode: 0xEF }}
         instructions {
           vendor_syntax {
             mnemonic: 'OUT'
             operands { name: 'DX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }
             operands { name: 'EAX' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'EF'
           x86_encoding_specification { legacy_prefixes {} opcode: 0xEF }})";
  TestTransform(AddOperandSizeOverridePrefix, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
