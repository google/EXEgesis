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

// This is an integration test for the simulator. It uses data from a real
// target. For unit tests, see
// `llvm_sim/framework:simulator_test` and
// `llvm_sim/components/...`.
#include "llvm_sim/x86/haswell.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/Target/X86/X86InstrInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm_sim/analysis/port_pressure.h"
#include "llvm_sim/framework/log_levels.h"
#include "llvm_sim/x86/faucon_lib.h"

namespace exegesis {
namespace simulator {
namespace {

class HaswellTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmParser();
  }

  void RunTestCase(const std::string& TestCase, int MaxNumCycles = 1000);
};

void HaswellTest::RunTestCase(const std::string& TestCase, int MaxNumCycles) {
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  const auto Simulator = CreateHaswellSimulator(*Context);

  const std::string FileName = std::string(getenv("TEST_SRCDIR")) +
                               "/__main__/llvm_sim/"
                               "x86/testdata/" +
                               TestCase;
  const auto Instructions =
      ParseAsmCodeFromFile(*Context, FileName, llvm::InlineAsm::AD_Intel);
  ASSERT_TRUE(!Instructions.empty());

  const BlockContext BlockContext(Instructions, true);
  const auto Log =
      Simulator->Run(BlockContext, /*MaxNumIterations=*/100, MaxNumCycles);

  llvm::outs() << Log->DebugString();
  for (const auto& Line : Log->Lines) {
    ASSERT_NE(Line.MsgTag, LogLevels::kWarning) << Line.Msg;
  }
}

TEST_F(HaswellTest, Test1) { RunTestCase("test1.s"); }
TEST_F(HaswellTest, Test2) { RunTestCase("test2.s"); }
TEST_F(HaswellTest, Test3) { RunTestCase("test3.s"); }
TEST_F(HaswellTest, Test4) { RunTestCase("test4.s"); }
TEST_F(HaswellTest, Test5) { RunTestCase("test5.s"); }
TEST_F(HaswellTest, Test6) { RunTestCase("test6.s"); }
TEST_F(HaswellTest, Test9) { RunTestCase("test9.s"); }
TEST_F(HaswellTest, DISABLED_Test10) { RunTestCase("test10.s", 0); }

}  // namespace
}  // namespace simulator
}  // namespace exegesis
