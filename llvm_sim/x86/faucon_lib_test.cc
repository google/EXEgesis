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
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm_sim/x86/constants.h"

namespace exegesis {
namespace simulator {
namespace {

class FauconUtilTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
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
  constexpr char kTestBinaryPath[] =
      "/__main__/llvm_sim/x86/testdata/testbinary";
  const std::string FileName =
      std::string(getenv("TEST_SRCDIR")) + kTestBinaryPath;
  EXPECT_THAT(GetIACAMarkedCode(FileName),
              testing::ElementsAreArray(kTestCodeBytes));
}

TEST_F(FauconUtilTest, ParseMCInsts) {
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions = ParseMCInsts(*Context, kTestCodeBytes);
  EXPECT_EQ(Instructions.size(), 6);
}

TEST_F(FauconUtilTest, ParseAsmCodeFromFile) {
  std::string FileName = getenv("TEST_SRCDIR");
  FileName += "/__main__/llvm_sim/x86/testdata/test1.s";
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions =
      ParseAsmCodeFromFile(*Context, FileName, llvm::InlineAsm::AD_Intel);
  EXPECT_EQ(Instructions.size(), 7);
}

TEST_F(FauconUtilTest, ParseAsmCodeFromString) {
  constexpr char AsmCode[] = R"asm(
      start:
        add eax, ecx
        mov r8d, ecx
        imul r8d, r8d
        add edx, r8d
        add ecx, 0x1
        cmp edi, ecx
        jne start
      )asm";
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions =
      ParseAsmCodeFromString(*Context, AsmCode, llvm::InlineAsm::AD_Intel);
  EXPECT_EQ(Instructions.size(), 7);
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

class TestMCInstPrinter : public llvm::MCInstPrinter {
 public:
  TestMCInstPrinter(const GlobalContext &Context)
      : llvm::MCInstPrinter(*Context.AsmInfo, *Context.InstrInfo,
                            *Context.RegisterInfo) {}

  void printInst(const llvm::MCInst *MI, llvm::raw_ostream &OS,
                 llvm::StringRef Annot,
                 const llvm::MCSubtargetInfo &STI) override {
    OS << "instruction";
  }

  void printRegName(llvm::raw_ostream &OS, unsigned RegNo) const {
    OS << "reg" << RegNo;
  }
};

TEST_F(FauconUtilTest, PrintTrace) {
  std::vector<BufferDescription> BufferDescriptions(
      {BufferDescription("A", IntelBufferIds::kAllocated),
       BufferDescription("B", IntelBufferIds::kIssuePort),
       BufferDescription("C"),
       BufferDescription("D", IntelBufferIds::kWriteback),
       BufferDescription("E", IntelBufferIds::kRetired)});
  SimulationLog Log(BufferDescriptions);
  Log.NumCycles = 21;
  Log.Iterations.push_back({/*EndCycle=*/18});
  SimulationLog::Line Line;

  Line.Cycle = 3;
  Line.BufferIndex = 0;
  Line.MsgTag = UopId::kTagName;
  Line.Msg = UopId::Format({{0, 0}, 0});
  Log.Lines.push_back(Line);

  Line.Cycle = 5;
  Line.BufferIndex = 1;
  Line.MsgTag = UopId::kTagName;
  Line.Msg = UopId::Format({{0, 0}, 0});
  Log.Lines.push_back(Line);

  Line.Cycle = 7;
  Line.BufferIndex = 2;
  Line.MsgTag = UopId::kTagName;
  Line.Msg = UopId::Format({{0, 0}, 0});
  Log.Lines.push_back(Line);

  Line.Cycle = 9;
  Line.BufferIndex = 3;
  Line.MsgTag = UopId::kTagName;
  Line.Msg = UopId::Format({{0, 0}, 0});
  Log.Lines.push_back(Line);

  Line.Cycle = 11;
  Line.BufferIndex = 4;
  Line.MsgTag = UopId::kTagName;
  Line.Msg = UopId::Format({{0, 0}, 0});
  Log.Lines.push_back(Line);

  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Instructions =
      ParseMCInsts(*Context, {0x01, 0xC8} /* add eax, ecx */);
  const BlockContext BlockContext(Instructions, true);

  std::string Out;
  llvm::raw_string_ostream OS{Out};
  TestMCInstPrinter AsmPrinter(*Context);
  PrintTrace(*Context, BlockContext, Log, AsmPrinter, OS);
  OS.flush();
  // clang-format off
  EXPECT_EQ(Out,
            "|it|in|Disassembly                                       :012345678901234567890\n"  // NOLINT
            "| 0| 0|instruction                                       :          |         |\n"  // NOLINT
            "|  |  |      uop 0                                       :   A-deeew-R        |\n"  // NOLINT
           );
  // clang-format on
}
}  // namespace
}  // namespace simulator
}  // namespace exegesis
