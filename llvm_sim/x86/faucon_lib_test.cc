// Copyright 2018 Google Inc.
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

#include "llvm_sim/x86/faucon_lib.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"

namespace exegesis {
namespace simulator {
namespace {

class FauconUtilTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86Disassembler();
    LLVMInitializeX86AsmParser();
  }
};

const uint8_t kTestCodeBytes[] = {
    0x01, 0xC8,              // add eax, ecx
    0x41, 0x89, 0xC8,        // mov r8d, ecx
    0x45, 0x0F, 0xAF, 0xC0,  // imul r8d, r8d
    0x44, 0x01, 0xC2,        // add edx, r8d
    0x83, 0xC1, 0x01,        // add ecx, 1
    0x39, 0xCF               // cmp edi, ecx
};

TEST_F(FauconUtilTest, GetIACAMarkedCode) {
  const std::string FileName = getenv("TEST_SRCDIR") +
                               "/__main__/llvm_sim/"
                               "x86/testdata/testbinary";
  EXPECT_THAT(GetIACAMarkedCode(FileName),
              testing::ElementsAreArray(kTestCodeBytes));
}

TEST_F(FauconUtilTest, ParseMCInsts) {
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions = ParseMCInsts(*Context, kTestCodeBytes);
  EXPECT_EQ(Instructions.size(), 6);
}

TEST_F(FauconUtilTest, ParseAsmCodeFromFile) {
  const std::string FileName = getenv("TEST_SRCDIR") +
                               "/__main__/llvm_sim/"
                               "x86/testdata/test1.s";
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions =
      ParseAsmCodeFromFile(*Context, FileName, llvm::InlineAsm::AD_Intel);
  EXPECT_EQ(Instructions.size(), 6);
}

TEST(TextTableTest, NoHeader) {
  TextTable Table(2, 3, false);
  Table.SetValue(0, 1, "a");
  Table.SetValue(0, 2, "bc");
  Table.SetValue(1, 0, "def");
  Table.SetValue(1, 1, "gh");
  Table.SetValue(1, 2, "i");
  std::string Out;
  llvm::raw_string_ostream OS{Out};
  Table.Render(OS);
  OS.flush();
  EXPECT_EQ(Out,
            "\n"
            "-----------------\n"
            "|     |  a | bc | \n"
            "| def | gh |  i | \n"
            "-----------------\n");
}

TEST(TextTableTest, Header) {
  TextTable Table(2, 3, true);
  Table.SetValue(0, 1, "a");
  Table.SetValue(0, 2, "bc");
  Table.SetValue(1, 0, "def");
  Table.SetValue(1, 1, "gh");
  Table.SetValue(1, 2, "i");
  std::string Out;
  llvm::raw_string_ostream OS{Out};
  Table.Render(OS);
  OS.flush();
  EXPECT_EQ(Out,
            "\n"
            "-----------------\n"
            "|     |  a | bc | \n"
            "-----------------\n"
            "| def | gh |  i | \n"
            "-----------------\n");
}

TEST(TextTableTest, TrailingValues) {
  TextTable Table(1, 2, false);
  Table.SetValue(0, 0, "a");
  Table.SetValue(0, 1, "b");
  Table.SetTrailingValue(0, "d\tef");
  std::string Out;
  llvm::raw_string_ostream OS{Out};
  Table.Render(OS);
  OS.flush();
  EXPECT_EQ(Out,
            "\n"
            "---------\n"
            "| a | b | d\tef\n"
            "---------\n");
}
}  // namespace
}  // namespace simulator
}  // namespace exegesis
