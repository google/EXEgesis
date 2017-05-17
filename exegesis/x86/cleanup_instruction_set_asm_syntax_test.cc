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

#include "exegesis/x86/cleanup_instruction_set_asm_syntax.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddIntelAsmSyntaxTest, StringMnemonic) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'BYTE PTR [RSI]' }
             operands { name: 'BYTE PTR [RDI]' }}})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'CMPS'
             operands { name: 'BYTE PTR [RSI]' }
             operands { name: 'BYTE PTR [RDI]' }}
           syntax {
             mnemonic: 'CMPSB'
             operands { name: 'BYTE PTR [RSI]' }
             operands { name: 'BYTE PTR [RDI]' }}})";
  TestTransform(AddIntelAsmSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddIntelAsmSyntaxTest, MovImm64) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'MOV'
             operands { name: 'r64' }
             operands { name: 'imm64' }}})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'MOV'
             operands { name: 'r64' }
             operands { name: 'imm64' }}
           syntax {
             mnemonic: 'MOVABS'
             operands { name: 'r64' }
             operands { name: 'imm64' }}})";
  TestTransform(AddIntelAsmSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddIntelAsmSyntaxTest, LSLR64) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'LSL'
             operands { name: 'r64' }
             operands { name: 'r32/m16' }}})";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'LSL'
             operands { name: 'r64' }
             operands { name: 'r32/m16' }}
           syntax {
             mnemonic: 'LSL'
             operands { name: 'r64' }
             operands { name: 'r64' }}})";
  TestTransform(AddIntelAsmSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
