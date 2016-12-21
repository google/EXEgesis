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

#include "cpu_instructions/x86/cleanup_instruction_set_encoding.h"

#include "gtest/gtest.h"
#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"

namespace cpu_instructions {
namespace x86 {
namespace {

TEST(AddMissingModRmAndImmediateSpecificationTest, Vmovd) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           description: 'Move doubleword from r32/m32 to xmm1.'
           vendor_syntax {
             mnemonic: 'VMOVD'
             operands { name: 'xmm1' }
             operands { name: 'r32' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W0 6E' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           description: 'Move doubleword from r32/m32 to xmm1.'
           vendor_syntax {
             mnemonic: 'VMOVD'
             operands { name: 'xmm1' }
             operands { name: 'r32' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W0 6E /r' })";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingModRmAndImmediateSpecificationTest, Kshiftlb) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'KSHIFTLB'
             operands { name: 'k1' }
             operands { name: 'k2' }
             operands { name: 'imm8' }}
           feature_name: 'AVX512DQ'
           encoding_scheme: 'RRI'
           binary_encoding: 'VEX.L0.66.0F3A.W0 32 /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'KSHIFTLB'
             operands { name: 'k1' }
             operands { name: 'k2' }
             operands { name: 'imm8' }}
           feature_name: 'AVX512DQ'
           encoding_scheme: 'RRI'
           binary_encoding: 'VEX.L0.66.0F3A.W0 32 /r ib' })";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingMemoryOffsetEncodingTest, AddOffset) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'AAD' operands { name: 'imm8' }}
           available_in_64_bit: false
           encoding_scheme: 'NP'
           binary_encoding: 'D5 ib' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'AL' }
                           operands { name: 'moffs8' }}
           encoding_scheme: 'FD'
           binary_encoding: 'A0' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'RAX'}
                           operands { name: 'moffs64' }}
           legacy_instruction: false
           encoding_scheme: 'FD'
           binary_encoding: 'REX.W + A1' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'AAD' operands { name: 'imm8' }}
           available_in_64_bit: false
           encoding_scheme: 'NP'
           binary_encoding: 'D5 ib' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'AL' }
                           operands { name: 'moffs8' }}
           encoding_scheme: 'FD'
           binary_encoding: 'A0 io' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'RAX' }
                           operands { name: 'moffs64' }}
           legacy_instruction: false
           encoding_scheme: 'FD'
           binary_encoding: 'REX.W + A1 io' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'AL' }
                           operands { name: 'moffs8' }}
           encoding_scheme: 'FD'
           binary_encoding: '67 A0 id' }
         instructions {
           vendor_syntax { mnemonic: 'MOV' operands { name: 'RAX' }
                           operands { name: 'moffs64' }}
           legacy_instruction: false
           encoding_scheme: 'FD'
           binary_encoding: '67 REX.W + A1 id' })";
  TestTransform(AddMissingMemoryOffsetEncoding, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingModRmAndImmediateSpecificationTest, NoChange) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           description: 'Adjust AX before division to number base imm8.'
           vendor_syntax {
             mnemonic: 'AAD'
             operands { name: 'imm8' }}
           available_in_64_bit: false
           encoding_scheme: 'NP'
           binary_encoding: 'D5 ib' }
         instructions {
           description: 'Move doubleword from r32/m32 to xmm1.'
           vendor_syntax {
             mnemonic: 'VMOVD'
             operands { name: 'xmm1' }
             operands { name: 'r32' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W0 6E /r' })";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(FixAndCleanUpBinaryEncodingSpecificationsOfSetInstructionsTest,
     SomeInstructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SETA' operands { name: 'r/m8' }}
           encoding_scheme: 'M'
           binary_encoding: '0F 97' }
         instructions {
           vendor_syntax { mnemonic: 'SETA' operands { name: 'r/m8' }}
           encoding_scheme: 'M'
           binary_encoding: 'REX + 0F 97' }
         instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands { name: 'BYTE PTR [RDI]' } operands { name: 'AL' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SETA' operands { name: 'r/m8' }}
           encoding_scheme: 'M'
           binary_encoding: '0F 97 /0' }
         instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands { name: 'BYTE PTR [RDI]' } operands { name: 'AL' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' })";
  TestTransform(FixAndCleanUpBinaryEncodingSpecificationsOfSetInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(FixBinaryEncodingSpecificationOfPopFsAndGsTest, SomeInstructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           description: 'Pop top of stack into FS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: '0F A1' }
         instructions {
           description: 'Pop top of stack into FS. Increment stack '
                        'pointer by 16 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '0F A1' }
         instructions {
           description: 'Pop top of stack into GS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: '0F A9' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           description: 'Pop top of stack into FS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: '0F A1' }
         instructions {
           description: 'Pop top of stack into FS. Increment stack '
                        'pointer by 16 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '66 0F A1' }
         instructions {
           description: 'Pop top of stack into GS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: '0F A9' }
         instructions {
           description: 'Pop top of stack into FS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: 'REX.W 0F A1' }
         instructions {
           description: 'Pop top of stack into GS. Increment stack '
                        'pointer by 64 bits.'
           vendor_syntax {
             mnemonic: 'POP'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           binary_encoding: 'REX.W 0F A9' })";
  TestTransform(FixBinaryEncodingSpecificationOfPopFsAndGs,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(FixBinaryEncodingSpecificationOfPushFsAndGsTest, SomeInstructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           description: 'Push FS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '0F A0' }
         instructions {
           description: 'Push GS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '0F A8' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           description: 'Push FS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '0F A0' }
         instructions {
           description: 'Push GS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '0F A8' }
         instructions {
           description: 'Push FS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '66 0F A0' }
         instructions {
           description: 'Push FS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'FS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: 'REX.W 0F A0' }
         instructions {
           description: 'Push GS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: '66 0F A8' }
         instructions {
           description: 'Push GS.'
           vendor_syntax {
             mnemonic: 'PUSH'
             operands { name: 'GS' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 16 }}
           encoding_scheme: 'NP'
           binary_encoding: 'REX.W 0F A8' })";
  TestTransform(FixBinaryEncodingSpecificationOfPushFsAndGs,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(FixBinaryEncodingSpecificationOfXBeginTest, SomeInstructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'VFMSUB231PS' operands { name: 'xmm0' }
                           operands { name: 'xmm1' } operands { name: 'm128' }}
           feature_name: 'FMA' encoding_scheme: 'A'
           binary_encoding: 'VEX.DDS.128.66.0F38.0 BA /r' }
         instructions {
           vendor_syntax { mnemonic: 'XBEGIN' operands { name: 'rel16' }}
           feature_name: 'RTM' encoding_scheme: 'A'
           binary_encoding: 'C7 F8' }
         instructions {
           vendor_syntax { mnemonic: 'XBEGIN' operands { name: 'rel32' }}
           feature_name: 'RTM' encoding_scheme: 'A'
           binary_encoding: 'C7 F8' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'VFMSUB231PS' operands { name: 'xmm0' }
                           operands { name: 'xmm1' } operands { name: 'm128' }}
           feature_name: 'FMA' encoding_scheme: 'A'
           binary_encoding: 'VEX.DDS.128.66.0F38.0 BA /r' }
         instructions {
           vendor_syntax { mnemonic: 'XBEGIN' operands { name: 'rel16' }}
           feature_name: 'RTM' encoding_scheme: 'A'
           binary_encoding: '66 C7 F8 cw' }
         instructions {
           vendor_syntax { mnemonic: 'XBEGIN' operands { name: 'rel32' }}
           feature_name: 'RTM' encoding_scheme: 'A'
           binary_encoding: 'C7 F8 cd' })";
  TestTransform(FixBinaryEncodingSpecificationOfXBegin, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixBinaryEncodingSpecificationsTest, SomeInstructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'VFMSUB231PS' operands { name: 'xmm0' }
                           operands { name: 'xmm1' } operands { name: 'm128' }}
           feature_name: 'FMA' encoding_scheme: 'A'
           binary_encoding: 'VEX.DDS.128.66.0F38.0 BA /r' }
         instructions {
           vendor_syntax { mnemonic: 'PCMPISTRI' operands { name: 'xmm1' }
                           operands { name: 'm128' } operands { name: 'imm8' }}
           feature_name: 'SSE4_2' encoding_scheme: 'RM'
           binary_encoding: '66 0F 3A 63 /r imm8' }
         instructions {
           vendor_syntax {
             mnemonic: 'PMOVSXBW'
             operands { name: 'xmm1' } operands { name: 'xmm2' }}
           feature_name: 'SSE4_1' encoding_scheme: 'RM'
           binary_encoding: '66 0f 38 20 /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'VFMSUB231PS' operands { name: 'xmm0' }
                           operands { name: 'xmm1' } operands { name: 'm128' }}
           feature_name: 'FMA' encoding_scheme: 'A'
           binary_encoding: 'VEX.DDS.128.66.0F38.W0 BA /r' }
         instructions {
           vendor_syntax { mnemonic: 'PCMPISTRI' operands { name: 'xmm1' }
                           operands { name: 'm128' } operands { name: 'imm8' }}
           feature_name: 'SSE4_2' encoding_scheme: 'RM'
           binary_encoding: '66 0F 3A 63 /r ib' }
         instructions {
           vendor_syntax {
             mnemonic: 'PMOVSXBW'
             operands { name: 'xmm1' } operands { name: 'xmm2' }}
           feature_name: 'SSE4_1' encoding_scheme: 'RM'
           binary_encoding: '66 0F 38 20 /r' })";
  TestTransform(FixBinaryEncodingSpecifications, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
