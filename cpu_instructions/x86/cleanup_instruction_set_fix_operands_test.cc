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

#include "cpu_instructions/x86/cleanup_instruction_set_fix_operands.h"

#include "src/google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {
namespace {

using ::cpu_instructions::util::IsInvalidArgument;
using ::cpu_instructions::util::Status;

TEST(FixOperandsOfCmpsAndMovsTest, Instructions) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'MOVS'
             operands { name: 'm16' encoding: IMPLICIT_ENCODING }
             operands { name: 'm16' }}
           encoding_scheme: 'NP' binary_encoding: 'A5' }
         instructions {
           vendor_syntax {
             mnemonic: 'MOVS'
             operands { name: 'm32' }
             operands { name: 'm32' }}
           encoding_scheme: 'NP' binary_encoding: 'A5' }
         instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'm8' }
             operands { name: 'm8'  encoding: IMPLICIT_ENCODING }}
           encoding_scheme: 'NP' binary_encoding: 'A6' }
         instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'm64' }
             operands { name: 'm64' }}
           legacy_instruction: false
           encoding_scheme: 'NP' binary_encoding: 'REX.W + A7' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'MOVS'
             operands {
               name: 'WORD PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_WRITE }
             operands {
               name: 'WORD PTR [RSI]' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: 'A5' }
         instructions {
           vendor_syntax {
             mnemonic: 'MOVS'
             operands { name: 'DWORD PTR [RDI]' usage: USAGE_WRITE }
             operands { name: 'DWORD PTR [RSI]' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: 'A5' }
         instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'BYTE PTR [RSI]' usage: USAGE_READ }
             operands {
               name: 'BYTE PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: 'A6' }
         instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'QWORD PTR [RSI]' usage: USAGE_READ }
             operands { name: 'QWORD PTR [RDI]' usage: USAGE_READ }}
           legacy_instruction: false
           encoding_scheme: 'NP' binary_encoding: 'REX.W + A7' })";
  TestTransform(FixOperandsOfCmpsAndMovs, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfInsAndOutsTest, Ins) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6C' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6D' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'INS'
             operands { name: 'BYTE PTR [RDI]' usage: USAGE_WRITE }
             operands { name: 'DX' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6C' }
         instructions {
           vendor_syntax {
             mnemonic: 'INS'
             operands { name: 'WORD PTR [RDI]' usage: USAGE_WRITE }
             operands { name: 'DX' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6D' })";
  TestTransform(FixOperandsOfInsAndOuts, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfInsAndOutsTest, Outs) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'OUTS'
             operands { name: 'DX' usage: USAGE_READ }
             operands { name: 'm16' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6F' }
         instructions {
           vendor_syntax {
             mnemonic: 'OUTS'
             operands { name: 'DX' usage: USAGE_READ }
             operands { name: 'm32' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6F' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'OUTS'
             operands { name: 'DX' usage: USAGE_READ }
             operands { name: 'WORD PTR [RSI]' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6F' }
         instructions {
           vendor_syntax {
             mnemonic: 'OUTS'
             operands { name: 'DX' usage: USAGE_READ }
             operands { name: 'DWORD PTR [RSI]' usage: USAGE_READ }}
           encoding_scheme: 'NP' binary_encoding: '6F' })";
  TestTransform(FixOperandsOfInsAndOuts, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsScasAndStosTest, Scas) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           binary_encoding: 'AE' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'SCAS'
             operands {
               name: 'AL' encoding: IMPLICIT_ENCODING  usage: USAGE_READ }
             operands {
               name: 'BYTE PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NP'
           binary_encoding: 'AE' })";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsScasAndStosTest, Stos) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm8' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' }
         instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm16' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm32' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOS' operands { name: 'm64' }}
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOSB' }
           encoding_scheme: 'NA'
           binary_encoding: 'AA' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands {
               name: 'BYTE PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
               name: 'AL' encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' }
         instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands {
               name: 'WORD PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
               name: 'AX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NA'
           binary_encoding: 'AB' }
         instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands {
               name: 'DWORD PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
               name: 'EAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NA'
           binary_encoding: 'AB' }
         instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands {
               name: 'QWORD PTR [RDI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands { name: 'RAX'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AB' }
         instructions {
           vendor_syntax { mnemonic: 'STOSB' }
           encoding_scheme: 'NA'
           binary_encoding: 'AA' })";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsAndStosTest, Lods) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'LODS' operands { name: 'm8' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AC' }
         instructions {
           vendor_syntax { mnemonic: 'LODS' operands { name: 'm16' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AD' }
         instructions {
           vendor_syntax { mnemonic: 'LODS' operands { name: 'm32' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AD' }
         instructions {
           vendor_syntax { mnemonic: 'LODS' operands { name: 'm64' }}
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AD' }
         instructions {
           vendor_syntax { mnemonic: 'LODSB' }
           encoding_scheme: 'NA'
           binary_encoding: 'AC' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'LODS'
             operands {
               name: 'AL' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
               name: 'BYTE PTR [RSI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ}}
           encoding_scheme: 'NA'
           binary_encoding: 'AC' }
         instructions {
           vendor_syntax {
             mnemonic: 'LODS'
             operands {
               name: 'AX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
               name: 'WORD PTR [RSI]'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NA'
           binary_encoding: 'AD' }
         instructions {
           vendor_syntax {
             mnemonic: 'LODS'
              operands {
                name: 'EAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
              operands {
                name: 'DWORD PTR [RSI]'
                encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           encoding_scheme: 'NA'
           binary_encoding: 'AD' }
         instructions {
           vendor_syntax {
             mnemonic: 'LODS'
             operands { name: 'RAX'
               encoding: IMPLICIT_ENCODING usage: USAGE_READ }
             operands {
              name: 'QWORD PTR [RSI]'
              encoding: IMPLICIT_ENCODING usage: USAGE_READ }}
           legacy_instruction: false
           encoding_scheme: 'NA'
           binary_encoding: 'REX.W + AD' }
         instructions {
           vendor_syntax { mnemonic: 'LODSB' }
           encoding_scheme: 'NA'
           binary_encoding: 'AC' })";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfVMovqTest, FixOperand) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VMOVQ'
             operands { name: 'xmm1' } operands { name: 'm64' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.F3.0F.WIG 7E /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VMOVQ'
             operands { name: 'xmm1' } operands { name: 'r/m64' }}
           feature_name: 'AVX'
           legacy_instruction: false
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W1 6E /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VMOVQ'
             operands { name: 'xmm1' } operands { name: 'xmm2' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.F3.0F.WIG 7E /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'VMOVQ' operands { name: 'xmm1' }
                           operands { name: 'xmm2/m64' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.F3.0F.WIG 7E /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'VMOVQ'
             operands { name: 'xmm1' } operands { name: 'r/m64' }}
           feature_name: 'AVX'
           legacy_instruction: false
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W1 6E /r' }
         instructions {
           vendor_syntax { mnemonic: 'VMOVQ' operands { name: 'xmm1' }
                           operands { name: 'xmm2/m64' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.F3.0F.WIG 7E /r' })";
  TestTransform(FixOperandsOfVMovq, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixRegOperandsTest, FixOperand) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'LAR' operands { name: 'r16' }
                           operands { name: 'r16' }}
           encoding_scheme: 'RM'
           binary_encoding: '0F 02 /r' }
         instructions {
           vendor_syntax { mnemonic: 'LAR' operands { name: 'reg' }
                           operands { name: 'r32' }}
           encoding_scheme: 'RM'
           binary_encoding: '0F 02 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'MOVMSKPS'
             operands { name: 'reg' } operands { name: 'xmm' }}
           feature_name: 'SSE'
           encoding_scheme: 'RM'
           binary_encoding: '0F 50 /r' }
         instructions {
           vendor_syntax { mnemonic: 'MOVQ' operands { name: 'm64' }
                           operands { name: 'mm' }}
           feature_name: 'MMX'
           legacy_instruction: false
           encoding_scheme: 'MR'
           binary_encoding: 'REX.W + 0F 7E /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'LAR' operands { name: 'r16' }
                           operands { name: 'r16' }}
           encoding_scheme: 'RM'
           binary_encoding: '0F 02 /r' }
         instructions {
           vendor_syntax { mnemonic: 'LAR' operands { name: 'r64' }
                           operands { name: 'r32' }}
           encoding_scheme: 'RM'
           binary_encoding: 'REX.W + 0F 02 /r' }
         instructions {
           vendor_syntax {
             mnemonic: 'MOVMSKPS'
             operands { name: 'r32' } operands { name: 'xmm' }}
           feature_name: 'SSE'
           encoding_scheme: 'RM'
           binary_encoding: '0F 50 /r' }
         instructions {
           vendor_syntax { mnemonic: 'MOVQ' operands { name: 'm64' }
                           operands { name: 'mm' }}
           feature_name: 'MMX'
           legacy_instruction: false
           encoding_scheme: 'MR'
           binary_encoding: 'REX.W + 0F 7E /r' }
         instructions {
           vendor_syntax { mnemonic: 'LAR' operands { name: 'r32' }
                           operands { name: 'r32' }}
           encoding_scheme: 'RM'
           binary_encoding: '0F 02 /r' })";
  TestTransform(FixRegOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixRegOperandsTest, UnexpectedMnemonic) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'LARfoo'
             operands { name: 'reg' } operands { name: 'r32' }}
           encoding_scheme: 'RM'
           binary_encoding: '0F 02 /r' })";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(kInstructionSetProto,
                                                    &instruction_set));
  const Status transform_status = FixRegOperands(&instruction_set);
  EXPECT_TRUE(IsInvalidArgument(transform_status)) << transform_status;
}

TEST(RemoveImplicitST0OperandTest, NoRemoval) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FCMOVE'
             operands { name: 'ST(0)' }
             operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'DA C8+i' }
         instructions {
           vendor_syntax { mnemonic: 'FCOM' }
           feature_name: 'X87'
           binary_encoding: 'D8 D1' })";
  TestTransform(RemoveImplicitST0Operand, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(RemoveImplicitST0OperandTest, RemoveSomeOperands) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FCMOVE'
             operands { name: 'ST(0)' }
             operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'DA C8+i' }
         instructions {
           vendor_syntax {
             mnemonic: 'FADD'
             operands { name: 'ST(0)' }
             operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'D8 C0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FCOM' }
           feature_name: 'X87'
           binary_encoding: 'D8 D1' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FCMOVE'
             operands { name: 'ST(0)' }
             operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'DA C8+i' }
         instructions {
           vendor_syntax { mnemonic: 'FADD' operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'D8 C0+i' }
         instructions {
           vendor_syntax { mnemonic: 'FCOM' }
           feature_name: 'X87'
           binary_encoding: 'D8 D1' })";
  TestTransform(RemoveImplicitST0Operand, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveImplicitXmm0OperandTest, NoRemoval) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'VFMADD132PD'
             operands { name: 'xmm0' }
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }}
           feature_name: 'FMA'
           encoding_scheme: 'A'
           binary_encoding: 'VEX.DDS.128.66.0F38.W1 98 /r' })";
  TestTransform(RemoveImplicitXmm0Operand, kInstructionSetProto,
                kInstructionSetProto);
}

// NOTE(user): All instructions using the implicit XMM0 use it as the last
// operand. Thus, we do not test any other case.
TEST(RemoveImplicitXmm0OperandTest, RemoveLastOperand) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'BLENDVPS'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }
             operands { name: '<XMM0>' }}
           feature_name: 'SSE4_1'
           encoding_scheme: 'RM0'
           binary_encoding: '66 0F 38 14 /r' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'BLENDVPS'
             operands { name: 'xmm1' }
             operands { name: 'xmm2' }}
           feature_name: 'SSE4_1'
           encoding_scheme: 'RM0'
           binary_encoding: '66 0F 38 14 /r' })";
  TestTransform(RemoveImplicitXmm0Operand, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RenameOperandsTest, NoRenaming) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FADD'
             operands { name: 'ST(i)' }
             operands { name: 'ST(0)' }}
           feature_name: 'X87'
           binary_encoding: 'DE C0+i' }
         instructions {
           vendor_syntax {
             mnemonic: 'MOV'
             operands { name: 'm8' }
             operands { name: 'r8' }}
           binary_encoding: '88 /r' })";
  TestTransform(RenameOperands, kInstructionSetProto, kInstructionSetProto);
}

TEST(RenameOperandsTest, InstructionWithST) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FADD'
             operands { name: 'ST(i)' }
             operands { name: 'ST' }}
           feature_name: 'X87'
           binary_encoding: 'DE C0+i' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FADD'
             operands { name: 'ST(i)' }
             operands { name: 'ST(0)' }}
           feature_name: 'X87'
           binary_encoding: 'DE C0+i' })";
  TestTransform(RenameOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RenameOperandsTest, InstructionWithM80Dec) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FBLD'
             operands { name: 'm80dec' }}
           feature_name: 'X87'
           binary_encoding: 'DF /4' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FBLD'
             operands { name: 'm80bcd' }}
           feature_name: 'X87'
           binary_encoding: 'DF /4' })";
  TestTransform(RenameOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
