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

#include <unordered_set>

#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;
using ::testing::Pointee;

const char kInstructionWithSingleSyntax[] = R"proto(
  vendor_syntax {
    mnemonic: "STMXCSR"
    operands {
      addressing_mode: INDIRECT_ADDRESSING
      encoding: MODRM_RM_ENCODING
      value_size_bits: 32
      name: "m32"
      usage: USAGE_WRITE
    }
  }
  feature_name: "SSE"
  available_in_64_bit: true
  legacy_instruction: true
  encoding_scheme: "M"
  raw_encoding_specification: "NP 0F AE /3"
  protection_mode: -1
  x86_encoding_specification {
    opcode: 4014
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 3
    legacy_prefixes {
      rex_w_prefix: PREFIX_IS_NOT_PERMITTED
      operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
    }
  })proto";

constexpr char kInstructionWithMultipleSyntaxes[] = R"proto(
  llvm_mnemonic: "STOSB"
  vendor_syntax { mnemonic: "STOSB" }
  vendor_syntax {
    mnemonic: "STOS"
    operands {
      addressing_mode: INDIRECT_ADDRESSING_BY_RDI
      encoding: IMPLICIT_ENCODING
      value_size_bits: 8
      name: "BYTE PTR [RDI]"
      usage: USAGE_READ
    }
    operands {
      addressing_mode: DIRECT_ADDRESSING
      encoding: IMPLICIT_ENCODING
      value_size_bits: 8
      name: "AL"
      usage: USAGE_READ
      register_class: GENERAL_PURPOSE_REGISTER_8_BIT
    }
  }
  available_in_64_bit: true
  legacy_instruction: true
  encoding_scheme: "NA"
  raw_encoding_specification: "AA"
  protection_mode: -1
  x86_encoding_specification {
    opcode: 170
    legacy_prefixes {
      rex_w_prefix: PREFIX_IS_IGNORED
      operand_size_override_prefix: PREFIX_IS_IGNORED
    }
  })proto";
constexpr char kInstructionWithNoSyntaxes[] = R"proto(
  llvm_mnemonic: "STOSB"
  available_in_64_bit: true
  legacy_instruction: true
  encoding_scheme: "NA"
  raw_encoding_specification: "AA"
  protection_mode: -1
  x86_encoding_specification {
    opcode: 170
    legacy_prefixes {
      rex_w_prefix: PREFIX_IS_IGNORED
      operand_size_override_prefix: PREFIX_IS_IGNORED
    }
  })proto";

TEST(InstructionSyntaxTest, BuildFromStrings) {
  constexpr struct {
    const char* input;
    const char* expected_proto;
    const char* expected_output;
  } kTestCases[] = {
      {"AAD", "mnemonic: 'AAD' ", "AAD"},
      {"ADD RAX, imm32",
       "mnemonic: 'ADD' operands { name: 'RAX' } operands { name: 'imm32' "
       "} ",
       "ADD RAX, imm32"},
      {"XOR RAX,RBX",
       "mnemonic: 'XOR' operands { name: 'RAX' } operands { name: 'RBX' } ",
       "XOR RAX, RBX"},
      {"VADDPD xmm1,xmm2,xmm3/m128",
       R"proto(mnemonic: 'VADDPD'
               operands { name: 'xmm1' }
               operands { name: 'xmm2' }
               operands { name: 'xmm3/m128' })proto",
       "VADDPD xmm1, xmm2, xmm3/m128"},
      {"\tVAESDEC\txmm1,xmm2,xmm3/m128",
       R"proto(mnemonic: 'VAESDEC'
               operands { name: 'xmm1' }
               operands { name: 'xmm2' }
               operands { name: 'xmm3/m128' })proto",
       "VAESDEC xmm1, xmm2, xmm3/m128"},
      {"   VFMADD132PDy ymm1, ymm2,  ymm3   ",
       R"proto(mnemonic: 'VFMADD132PDy'
               operands { name: 'ymm1' }
               operands { name: 'ymm2' }
               operands { name: 'ymm3' })proto",
       "VFMADD132PDy ymm1, ymm2, ymm3"},
      {"LOCK MOV", "mnemonic: 'LOCK MOV' ", "LOCK MOV"},
      {"REPNE MOVS", "mnemonic: 'REPNE MOVS' ", "REPNE MOVS"},
      {"REP MOVS BYTE PTR [RDI], BYTE PTR [RSI]",
       R"proto(mnemonic: 'REP MOVS'
               operands { name: 'BYTE PTR [RDI]' }
               operands { name: 'BYTE PTR [RSI]' })proto",
       "REP MOVS BYTE PTR [RDI], BYTE PTR [RSI]"},
      {"REP ", "mnemonic: 'REP' ", "REP"},
      {"vpgatherqq %ymm2, (%rsp,%ymm12,8), %ymm1",
       R"proto(mnemonic: 'vpgatherqq'
               operands { name: '%ymm2' }
               operands { name: '(%rsp,%ymm12,8)' }
               operands { name: '%ymm1' })proto",
       "vpgatherqq %ymm2, (%rsp,%ymm12,8), %ymm1"},
      {"VPADDB xmm1 {k1} {z}, xmm2, XMMWORD PTR [RSI]",
       R"proto(mnemonic: 'VPADDB'
               operands {
                 name: 'xmm1'
                 tags { name: 'k1' }
                 tags { name: 'z' }
               }
               operands { name: 'xmm2' }
               operands { name: 'XMMWORD PTR [RSI]' })proto",
       "VPADDB xmm1 {k1} {z}, xmm2, XMMWORD PTR [RSI]"},
      {"VPADDB xmmword ptr [RSI + 4*RBP - 69] {k1} {z}, xmm2, xmm3",
       R"proto(mnemonic: 'VPADDB'
               operands {
                 name: 'xmmword ptr [RSI + 4*RBP - 69]'
                 tags { name: 'k1' }
                 tags { name: 'z' }
               }
               operands { name: 'xmm2' }
               operands { name: 'xmm3' })proto",
       "VPADDB xmmword ptr [RSI + 4*RBP - 69] {k1} {z}, xmm2, xmm3"},
      {"vpaddb %xmm3, %xmm2, -69(%rsi, %rbp, 4) {k1} {z}",
       R"proto(mnemonic: 'vpaddb'
               operands { name: '%xmm3' }
               operands { name: '%xmm2' }
               operands {
                 name: '-69(%rsi, %rbp, 4)'
                 tags { name: 'k1' }
                 tags { name: 'z' }
               })proto",
       "vpaddb %xmm3, %xmm2, -69(%rsi, %rbp, 4) {k1} {z}"},
      {"VCMPSD k1 {k2}, xmm2, xmm3, {sae}, 0x11",
       R"proto(mnemonic: 'VCMPSD'
               operands {
                 name: 'k1'
                 tags { name: 'k2' }
               }
               operands { name: 'xmm2' }
               operands { name: 'xmm3' }
               operands { tags { name: 'sae' } }
               operands { name: '0x11' })proto",
       "VCMPSD k1 {k2}, xmm2, xmm3, {sae}, 0x11"},
      {"VADDPD zmm1 {k1} {z}, zmm2, zmm3, {rd-sae}",
       R"proto(mnemonic: "VADDPD"
               operands {
                 name: "zmm1"
                 tags { name: "k1" }
                 tags { name: "z" }
               }
               operands { name: "zmm2" }
               operands { name: "zmm3" }
               operands { tags { name: "rd-sae" } })proto",
       "VADDPD zmm1 {k1} {z}, zmm2, zmm3, {rd-sae}"}};
  for (const auto& test_case : kTestCases) {
    const InstructionFormat proto = ParseAssemblyStringOrDie(test_case.input);
    EXPECT_THAT(proto, EqualsProto(test_case.expected_proto));
    EXPECT_EQ(ConvertToCodeString(proto), test_case.expected_output);
  }
}

TEST(GetUniqueVendorSyntaxOrDieTest, GetSyntax) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(kInstructionWithSingleSyntax);
  EXPECT_THAT(GetUniqueVendorSyntaxOrDie(instruction),
              EqualsProto(instruction.vendor_syntax(0)));
}

TEST(GetUniqueVendorSyntaxOrDieDeathTest, NoSyntax) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(kInstructionWithNoSyntaxes);
  EXPECT_DEATH(GetUniqueVendorSyntaxOrDie(instruction), "");
}

TEST(GetUniqueVendorSyntaxOrDieDeathTest, MultipleSyntaxes) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_DEATH(GetUniqueVendorSyntaxOrDie(instruction), "");
}

TEST(GetAnyVendorSyntaxOrDieTest, GetSyntax) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_THAT(GetAnyVendorSyntaxOrDie(instruction),
              EqualsProto(instruction.vendor_syntax(0)));
}

TEST(GetAnyVendorSyntaxOrDieDeathTest, NoSyntaxes) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(kInstructionWithNoSyntaxes);
  EXPECT_DEATH(GetVendorSyntaxWithMostOperandsOrDie(instruction), "");
}

TEST(GetVendorSyntaxWithMostOperandsOrDieTest, GetSyntax) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_THAT(GetVendorSyntaxWithMostOperandsOrDie(instruction),
              EqualsProto(instruction.vendor_syntax(1)));
}

TEST(GetVendorSyntaxWithMostOperandsOrDieDeathTest, NoSyntaxes) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(kInstructionWithNoSyntaxes);
  EXPECT_DEATH(GetVendorSyntaxWithMostOperandsOrDie(instruction), "");
}

TEST(GetOrAddUniqueVendorSyntaxOrDieTest, AddOrGetValue) {
  constexpr int kNumTrials = 10;
  InstructionProto instruction;
  InstructionFormat* const added_format =
      GetOrAddUniqueVendorSyntaxOrDie(&instruction);
  ASSERT_EQ(instruction.vendor_syntax_size(), 1);
  EXPECT_EQ(added_format, instruction.mutable_vendor_syntax(0));
  for (int i = 0; i < kNumTrials; ++i) {
    InstructionFormat* const returned_format =
        GetOrAddUniqueVendorSyntaxOrDie(&instruction);
    ASSERT_EQ(instruction.vendor_syntax_size(), 1);
    EXPECT_EQ(returned_format, instruction.mutable_vendor_syntax(0));
  }
}

TEST(GetOrAddUniqueVendorSyntaxOrDieDeathTest, MultipleSyntaxes) {
  InstructionProto instruction;
  instruction.add_vendor_syntax();
  instruction.add_vendor_syntax();
  EXPECT_DEATH(GetOrAddUniqueVendorSyntaxOrDie(&instruction), "");
}

TEST(HasMnemonicInVendorSyntaxTest, HasMnemonic) {
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_TRUE(HasMnemonicInVendorSyntax(instruction, "STOS"));
  EXPECT_TRUE(HasMnemonicInVendorSyntax(instruction, "STOSB"));
  EXPECT_FALSE(HasMnemonicInVendorSyntax(instruction, "XOR"));
}

TEST(ContainsVendorSyntaxMnemonicTest, ContainsMnemonic) {
  absl::flat_hash_set<std::string> kMnemonics1 = {"STOS", "MOV"};
  absl::flat_hash_set<std::string> kMnemonics2 = {"XOR", "ADD"};
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_TRUE(ContainsVendorSyntaxMnemonic(kMnemonics1, instruction));
  EXPECT_FALSE(ContainsVendorSyntaxMnemonic(kMnemonics2, instruction));
}

TEST(FindByVendorSyntaxMnemonicOrNullTest, GetValue) {
  absl::node_hash_map<std::string, int> kValues1 = {{"MOVS", 0}, {"STOS", 1}};
  absl::node_hash_map<std::string, int> kValues2 = {{"XOR", 0}, {"ADD", 1}};
  const InstructionProto instruction =
      ParseProtoFromStringOrDie<InstructionProto>(
          kInstructionWithMultipleSyntaxes);
  EXPECT_THAT(FindByVendorSyntaxMnemonicOrNull(kValues1, instruction),
              Pointee(1));
  EXPECT_EQ(FindByVendorSyntaxMnemonicOrNull(kValues2, instruction), nullptr);
}

}  // namespace
}  // namespace exegesis
