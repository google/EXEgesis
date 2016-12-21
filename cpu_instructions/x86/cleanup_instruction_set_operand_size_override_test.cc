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

#include "cpu_instructions/x86/cleanup_instruction_set_operand_size_override.h"

#include "gtest/gtest.h"
#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"

namespace cpu_instructions {
namespace x86 {
namespace {

TEST(AddOperandSizeOverrideToInstructionsWithImplicitOperandsTest, AddPrefix) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'STOSQ' }
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOSW' }
           encoding_scheme: 'NA'
           binary_encoding: 'AB' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'STOSQ' }
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOSW' }
           encoding_scheme: 'NA'
           binary_encoding: '66 AB' })";
  TestTransform(AddOperandSizeOverrideToInstructionsWithImplicitOperands,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverrideToSpecialCaseInstructionsTest, AddPrefix) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: '0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r32/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: '0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r64/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: 'REX.W + 0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           binary_encoding: '58+ rw' }
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r64' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 64 }}
           legacy_instruction: false
           encoding_scheme: 'O'
           binary_encoding: '58+ rd' }
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'RM'
           binary_encoding: 'F2 0F 38 F1 /r' }
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'RM'
           binary_encoding: 'F2 0F 38 F1 /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: '66 0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r32/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: '0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'SMSW'
             operands { name: 'r64/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'M'
           binary_encoding: 'REX.W + 0F 01 /4' }
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r16' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'O'
           binary_encoding: '66 58+ rw' }
         instructions {
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'r64' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 64 }}
           legacy_instruction: false
           encoding_scheme: 'O'
           binary_encoding: '58+ rd' }
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'RM'
           binary_encoding: '66 F2 0F 38 F1 /r' }
         instructions {
           vendor_syntax { mnemonic: 'CRC32'
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 32 }
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'RM'
           binary_encoding: 'F2 0F 38 F1 /r' })";
  TestTransform(AddOperandSizeOverrideToSpecialCaseInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddOperandSizeOverridePrefixTest, AddPrefix) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm16' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'MI'
           binary_encoding: '81 /2 iw' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'MI'
           binary_encoding: '81 /2 id' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           binary_encoding: '83 /2 ib' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           binary_encoding: '83 /2 ib' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m64' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 64 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           legacy_instruction: false
           encoding_scheme: 'MI'
           binary_encoding: 'REX.W + 81 /2 id' }
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
           binary_encoding: 'EF' }
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
           binary_encoding: 'EF' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm16' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'MI'
           binary_encoding: '66 81 /2 iw' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           encoding_scheme: 'MI'
           binary_encoding: '81 /2 id' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m16' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 16 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           binary_encoding: '66 83 /2 ib' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m32' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }
             operands { name: 'imm8' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'MI'
           binary_encoding: '83 /2 ib' }
         instructions {
           vendor_syntax {
             mnemonic: 'ADC'
             operands { name: 'r/m64' addressing_mode: ANY_ADDRESSING_MODE
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 64 }
             operands { name: 'imm32' addressing_mode: NO_ADDRESSING
                        encoding: IMMEDIATE_VALUE_ENCODING
                        value_size_bits: 32 }}
           legacy_instruction: false
           encoding_scheme: 'MI'
           binary_encoding: 'REX.W + 81 /2 id' }
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
           binary_encoding: '66 EF' }
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
           binary_encoding: 'EF' })";
  TestTransform(AddOperandSizeOverridePrefix, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
