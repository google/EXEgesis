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

#include "exegesis/util/instruction_syntax.h"

#include "base/macros.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace cpu_instructions {
namespace {

using ::cpu_instructions::testing::EqualsProto;

TEST(InstructionSyntaxTest, BuildFromStrings) {
  constexpr struct {
    const char* input;
    const char* expected_proto;
    const char* expected_output;
  } kTestCases[] = {
      {"AAD", "mnemonic: 'AAD' ", "AAD"},
      {"ADD RAX,imm32",
       "mnemonic: 'ADD' operands { name: 'RAX' } operands { name: 'imm32' "
       "} ",
       "ADD RAX,imm32"},
      {"XOR RAX,RBX",
       "mnemonic: 'XOR' operands { name: 'RAX' } operands { name: 'RBX' } ",
       "XOR RAX,RBX"},
      {"VADDPD xmm1,xmm2,xmm3/m128",
       R"(mnemonic: 'VADDPD' operands { name: 'xmm1' } operands { name: 'xmm2' }
          operands { name: 'xmm3/m128' })",
       "VADDPD xmm1,xmm2,xmm3/m128"},
      {"\tVAESDEC\txmm1,xmm2,xmm3/m128",
       R"(mnemonic: 'VAESDEC' operands { name: 'xmm1' } operands { name: 'xmm2' }
          operands { name: 'xmm3/m128' })",
       "VAESDEC xmm1,xmm2,xmm3/m128"},
      {"   VFMADD132PDy ymm1, ymm2,  ymm3   ",
       R"(mnemonic: 'VFMADD132PDy' operands { name: 'ymm1' }
          operands { name: 'ymm2' } operands { name: 'ymm3' })",
       "VFMADD132PDy ymm1,ymm2,ymm3"},
      {"LOCK MOV", "mnemonic: 'LOCK MOV' ", "LOCK MOV"},
      {"REPNE MOVS", "mnemonic: 'REPNE MOVS' ", "REPNE MOVS"},
      {"REP MOVS BYTE PTR [RDI], BYTE PTR [RSI]",
       R"(mnemonic: 'REP MOVS' operands { name: 'BYTE PTR [RDI]' }
          operands { name: 'BYTE PTR [RSI]' })",
       "REP MOVS BYTE PTR [RDI],BYTE PTR [RSI]"},
      {"REP ", "mnemonic: 'REP' ", "REP"},
      {"vpgatherqq %ymm2,(%rsp,%ymm12,8),%ymm1",
       R"(mnemonic: 'vpgatherqq' operands { name: '%ymm2' }
          operands { name: '(%rsp,%ymm12,8)' } operands { name: '%ymm1' })",
       "vpgatherqq %ymm2,(%rsp,%ymm12,8),%ymm1"}};
  for (const auto& test_case : kTestCases) {
    const InstructionFormat proto = ParseAssemblyStringOrDie(test_case.input);
    EXPECT_THAT(proto, EqualsProto(test_case.expected_proto));
    EXPECT_EQ(ConvertToCodeString(proto), test_case.expected_output);
  }
}

}  // namespace
}  // namespace cpu_instructions
