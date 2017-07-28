// Copyright 2017 Google Inc.
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

#include "exegesis/llvm/assembler_disassembler.h"

#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;

bool AssemblyDisassemblyOK(const string& asm_code, const string& llvm_mnemonic,
                           const string& intel_format, const string& att_format,
                           llvm::InlineAsm::AsmDialect asm_dialect) {
  AssemblerDisassembler asm_disasm;
  EXPECT_TRUE(asm_disasm.AssembleDisassemble(asm_code, asm_dialect));
  EXPECT_EQ(llvm_mnemonic, asm_disasm.llvm_mnemonic());
  EXPECT_THAT(asm_disasm.intel_format(), EqualsProto(intel_format));
  EXPECT_THAT(asm_disasm.att_format(), EqualsProto(att_format));
  return true;
}

bool DisassemblyOK(const string& binary, const string& llvm_mnemonic,
                   const string& intel_format, const string& att_format) {
  AssemblerDisassembler asm_disasm;
  const auto parsed_binary = ParseBinaryInstructionAndPadWithNops(binary);
  EXPECT_TRUE(!parsed_binary.empty());
  EXPECT_TRUE(asm_disasm.Disassemble(parsed_binary.data()));
  EXPECT_EQ(llvm_mnemonic, asm_disasm.llvm_mnemonic());
  EXPECT_THAT(asm_disasm.intel_format(), EqualsProto(intel_format));
  EXPECT_THAT(asm_disasm.att_format(), EqualsProto(att_format));
  return true;
}

TEST(AssemblerDisassemblerTest, MovEax_Intel) {
  EXPECT_TRUE(AssemblyDisassemblyOK(
      "mov eax,0x12345678", "MOV32ri",
      "mnemonic: 'mov' "
      "operands { name: 'eax' } operands { name: '0x12345678' }",
      "mnemonic: 'movl' "
      "operands { name: '$0x12345678' } operands { name: '%eax' }",
      llvm::InlineAsm::AD_Intel));
}

TEST(AssemblerDisassemblerTest, MovRax_Intel) {
  EXPECT_TRUE(AssemblyDisassemblyOK(
      "movabs rax,0x1234567890ABCDEF", "MOV64ri",
      "mnemonic: 'movabs' "
      "operands { name: 'rax' } operands { name: '0x1234567890abcdef' }",
      "mnemonic: 'movabsq' "
      "operands { name: '$0x1234567890abcdef' } operands { name: '%rax' }",
      llvm::InlineAsm::AD_Intel));
}

TEST(AssemblerDisassemblerTest, MovEaxBinary) {
  EXPECT_TRUE(DisassemblyOK(
      "B8 78 56 34 12", "MOV32ri",
      "mnemonic: 'mov' "
      "operands { name: 'eax' } operands { name: '0x12345678' }",
      "mnemonic: 'movl' "
      "operands { name: '$0x12345678' } operands { name: '%eax' }"));
}

TEST(AssemblerDisassemblerTest, MovRaxBinary) {
  EXPECT_TRUE(DisassemblyOK(
      "0x48,0xb8,0xef,0xcd,0xab,0x90,0x78,0x56,0x34,0x12", "MOV64ri",
      "mnemonic: 'movabs' "
      "operands { name: 'rax' } operands { name: '0x1234567890abcdef' }",
      "mnemonic: 'movabsq' "
      "operands { name: '$0x1234567890abcdef' } operands { name: '%rax' }"));
}

TEST(AssemblerDisassemblerTest, MovEax_Att) {
  EXPECT_TRUE(AssemblyDisassemblyOK(
      "movl $$0x12345678, %eax", "MOV32ri",
      "mnemonic: 'mov' "
      "operands { name: 'eax' } operands { name: '0x12345678' }",
      "mnemonic: 'movl' "
      "operands { name: '$0x12345678' } operands { name: '%eax' }",
      llvm::InlineAsm::AD_ATT));
}

TEST(AssemblerDisassemblerTest, MovRax_Att) {
  EXPECT_TRUE(AssemblyDisassemblyOK(
      "movabsq $$0x1234567890ABCDEF, %rax", "MOV64ri",
      "mnemonic: 'movabs' "
      "operands { name: 'rax' } operands { name: '0x1234567890abcdef' }",
      "mnemonic: 'movabsq' "
      "operands { name: '$0x1234567890abcdef' } operands { name: '%rax' }",
      llvm::InlineAsm::AD_ATT));
}

}  // namespace
}  // namespace exegesis
