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

#include "cpu_instructions/x86/cleanup_instruction_set_removals.h"

#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace cpu_instructions {
namespace x86 {
namespace {

TEST(RemoveDuplicateInstructionsTest, RemoveThem) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }
             operands { name: 'xmm3/m128' }}
           feature_name: 'AVX'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }
             operands { name: 'xmm3/m128' }}
           feature_name: 'AVX'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' })";
  TestTransform(RemoveDuplicateInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveDuplicateVPCmpEqQTest, NoRemoval) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }
             operands { name: 'xmm3/m128' }}
           feature_name: 'AVX'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'ymm1' }
             operands { name: 'ymm2' }
             operands { name: 'ymm3/m256' }}
           feature_name: 'AVX2'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.256.66.0F38.WIG 29 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VPCMPEQQ'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }
             operands { name: 'xmm3/m128' }}
           feature_name: 'AVX'
           encoding_scheme: 'RVM'
           raw_encoding_specification: 'VEX.NDS.128.66.0F38.WIG 29 /r' })";
  TestTransform(RemoveDuplicateInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveInstructionsWaitingForFpuSyncTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FCHS' }
           feature_name: 'X87'
           raw_encoding_specification: 'D9 E0' }
         instructions {
           vendor_syntax { mnemonic: 'FCLEX' }
           feature_name: 'X87'
           raw_encoding_specification: '9B DB E2' }
         instructions {
           vendor_syntax { mnemonic: 'FWAIT' }
           encoding_scheme: 'NP'
           raw_encoding_specification: '9B' }
         instructions {
           vendor_syntax { mnemonic: 'FNSAVE' operands { name: 'm108byte' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD /6' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FCHS' }
           feature_name: 'X87'
           raw_encoding_specification: 'D9 E0' }
         instructions {
           vendor_syntax { mnemonic: 'FWAIT' }
           encoding_scheme: 'NP'
           raw_encoding_specification: '9B' }
         instructions {
           vendor_syntax { mnemonic: 'FNSAVE' operands { name: 'm108byte' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD /6' })";
  TestTransform(RemoveInstructionsWaitingForFpuSync, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveRepAndRepneInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'REP STOS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'F3 AA' }
         instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'AE' }
         instructions {
           vendor_syntax {
             mnemonic: 'REPNE CMPS'
             operands { name: 'm8' }
             operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'F2 A6' }
         instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm8' }}
           encoding_scheme: 'NA'
           raw_encoding_specification: 'AA' }
         instructions {
           vendor_syntax { mnemonic: 'REPE SCAS' operands { name: 'm8' }}
           legacy_instruction: false
           encoding_scheme: 'NP'
           raw_encoding_specification: 'F3 REX.W AE' }
         instructions {
           vendor_syntax { mnemonic: 'CMPSB' }
           encoding_scheme: 'NP'
           raw_encoding_specification: 'A6' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'AE' }
         instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm8' }}
           encoding_scheme: 'NA'
           raw_encoding_specification: 'AA' }
         instructions {
           vendor_syntax { mnemonic: 'CMPSB' }
           encoding_scheme: 'NP'
           raw_encoding_specification: 'A6' })";
  TestTransform(RemoveRepAndRepneInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveNonEncodableInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'AAS' }
           available_in_64_bit: false
           encoding_scheme: 'NP'
           raw_encoding_specification: '3F' }
         instructions { vendor_syntax {
           mnemonic: 'CALL' operands { name: 'm16:32' }}
           encoding_scheme: 'M'
           raw_encoding_specification: 'FF /3' }
         instructions {
           description: 'Clears TS flag in CR0.'
           vendor_syntax { mnemonic: 'CLTS' }
           encoding_scheme: 'NP'
           raw_encoding_specification: '0F 06' }
         instructions {
           vendor_syntax { mnemonic: 'DEC' operands { name: 'r16' }}
           available_in_64_bit: false
           encoding_scheme: 'O'
           raw_encoding_specification: '66 48+rw' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'CALL' operands { name: 'm16:32' }}
           encoding_scheme: 'M'
           raw_encoding_specification: 'FF /3' }
         instructions {
           description: 'Clears TS flag in CR0.'
           vendor_syntax { mnemonic: 'CLTS' }
           encoding_scheme: 'NP'
           raw_encoding_specification: '0F 06' })";
  TestTransform(RemoveNonEncodableInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveSpecialCaseInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FUCOM' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD E0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FUCOM' }
           feature_name: 'X87'
           raw_encoding_specification: 'DD E1'
         }
         instructions {
           vendor_syntax { mnemonic: 'FUCOMI'
             operands { name: 'ST' } operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DB E8+i' }
         instructions {
           vendor_syntax { mnemonic: 'FDIVRP' }
           feature_name: 'X87'
           raw_encoding_specification: 'DE F1'
         }
         instructions {
           vendor_syntax { mnemonic: 'FFREE' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD C0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FADDP' }
           feature_name: 'X87'
           raw_encoding_specification: 'DE C1' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FUCOM' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD E0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FUCOMI'
             operands { name: 'ST' } operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DB E8+i' }
         instructions {
           vendor_syntax { mnemonic: 'FFREE' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD C0+i' })";
  TestTransform(RemoveSpecialCaseInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveUndefinedInstructionsTest, RemoveSomeInstructions) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FUCOM' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD E0+i' }
         instructions {
           vendor_syntax { mnemonic: "UD0" }
           raw_encoding_specification: "0F FF" }
         instructions {
           vendor_syntax { mnemonic: 'FFREE' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD C0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FADDP' }
           feature_name: 'X87'
           raw_encoding_specification: 'DE C1' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'FUCOM' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD E0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FFREE' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           raw_encoding_specification: 'DD C0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FADDP' }
           feature_name: 'X87'
           raw_encoding_specification: 'DE C1' })";
  TestTransform(RemoveUndefinedInstructions, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
