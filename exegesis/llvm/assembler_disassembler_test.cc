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

#include "exegesis/llvm/assembler_disassembler.pb.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::proto::IgnoringFields;

void CheckAssemblyDisassemblyOK(const std::string& asm_code,
                                llvm::InlineAsm::AsmDialect asm_dialect,
                                const std::string& expected) {
  AssemblerDisassembler asm_disasm;
  const auto result = asm_disasm.AssembleDisassemble(asm_code, asm_dialect);
  EXPECT_OK(result);
  EXPECT_THAT(
      result.ValueOrDie(),
      IgnoringFields({"exegesis.AssemblerDisassemblerResult.llvm_opcode",
                      "exegesis.AssemblerDisassemblerResult.binary_encoding"},
                     EqualsProto(expected)));
}

void CheckDisassemblyOK(const std::string& binary,
                        const std::string& expected) {
  AssemblerDisassembler asm_disasm;
  const auto result = asm_disasm.AssembleDisassemble(
      binary,
      AssemblerDisassemblerInterpretation::HUMAN_READABLE_BINARY_OR_INTEL_ASM);
  EXPECT_EQ(result.second,
            AssemblerDisassemblerInterpretation::HUMAN_READABLE_BINARY);
  EXPECT_OK(result.first);
  EXPECT_THAT(
      result.first.ValueOrDie(),
      IgnoringFields({"exegesis.AssemblerDisassemblerResult.llvm_opcode",
                      "exegesis.AssemblerDisassemblerResult.binary_encoding"},
                     EqualsProto(expected)));
}

TEST(AssemblerDisassemblerTest, MovEax_Intel) {
  CheckAssemblyDisassemblyOK("mov eax,0x12345678", llvm::InlineAsm::AD_Intel,
                             R"proto(
                               llvm_mnemonic: 'MOV32ri'
                               intel_syntax {
                                 mnemonic: 'mov'
                                 operands { name: 'eax' }
                                 operands { name: '0x12345678' }
                               }
                               att_syntax {
                                 mnemonic: 'movl'
                                 operands { name: '$0x12345678' }
                                 operands { name: '%eax' }
                               })proto");
}

TEST(AssemblerDisassemblerTest, MovRax_Intel) {
  CheckAssemblyDisassemblyOK("movabs rax,0x1234567890ABCDEF",
                             llvm::InlineAsm::AD_Intel,
                             R"proto(
                               llvm_mnemonic: 'MOV64ri'
                               intel_syntax {
                                 mnemonic: 'movabs'
                                 operands { name: 'rax' }
                                 operands { name: '0x1234567890abcdef' }
                               }
                               att_syntax {
                                 mnemonic: 'movabsq'
                                 operands { name: '$0x1234567890abcdef' }
                                 operands { name: '%rax' }
                               })proto");
}

TEST(AssemblerDisassemblerTest, MovEaxBinary) {
  CheckDisassemblyOK("B8 78 56 34 12", R"proto(
    llvm_mnemonic: 'MOV32ri'
    intel_syntax {
      mnemonic: 'mov'
      operands { name: 'eax' }
      operands { name: '0x12345678' }
    }
    att_syntax {
      mnemonic: 'movl'
      operands { name: '$0x12345678' }
      operands { name: '%eax' }
    })proto");
}

TEST(AssemblerDisassemblerTest, MovRaxBinary) {
  CheckDisassemblyOK("0x48,0xb8,0xef,0xcd,0xab,0x90,0x78,0x56,0x34,0x12",
                     R"proto(
                       llvm_mnemonic: 'MOV64ri'
                       intel_syntax {
                         mnemonic: 'movabs'
                         operands { name: 'rax' }
                         operands { name: '0x1234567890abcdef' }
                       }
                       att_syntax {
                         mnemonic: 'movabsq'
                         operands { name: '$0x1234567890abcdef' }
                         operands { name: '%rax' }
                       })proto");
}

TEST(AssemblerDisassemblerTest, MovEax_Att) {
  CheckAssemblyDisassemblyOK("movl $$0x12345678, %eax", llvm::InlineAsm::AD_ATT,
                             R"proto(
                               llvm_mnemonic: 'MOV32ri'
                               intel_syntax {
                                 mnemonic: 'mov'
                                 operands { name: 'eax' }
                                 operands { name: '0x12345678' }
                               }
                               att_syntax {
                                 mnemonic: 'movl'
                                 operands { name: '$0x12345678' }
                                 operands { name: '%eax' }
                               })proto");
}

TEST(AssemblerDisassemblerTest, MovRax_Att) {
  CheckAssemblyDisassemblyOK("movabsq $$0x1234567890ABCDEF, %rax",
                             llvm::InlineAsm::AD_ATT,
                             R"proto(
                               llvm_mnemonic: 'MOV64ri'
                               intel_syntax {
                                 mnemonic: 'movabs'
                                 operands { name: 'rax' }
                                 operands { name: '0x1234567890abcdef' }
                               }
                               att_syntax {
                                 mnemonic: 'movabsq'
                                 operands { name: '$0x1234567890abcdef' }
                                 operands { name: '%rax' }
                               })proto");
}

}  // namespace
}  // namespace exegesis
