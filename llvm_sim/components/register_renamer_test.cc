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

#include "llvm_sim/components/register_renamer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lib/Target/X86/X86InstrInfo.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using llvm::X86::AL;
using llvm::X86::CH;
using llvm::X86::CL;
using llvm::X86::CX;
using llvm::X86::DL;
using llvm::X86::EAX;
using llvm::X86::ECX;
using llvm::X86::EDI;
using llvm::X86::EDX;
using llvm::X86::ESI;
using llvm::X86::R12;
using llvm::X86::R13;
using llvm::X86::RCX;
using testing::ElementsAre;
using testing::Field;
using testing::UnorderedElementsAre;

class RegisterRenamerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    Context_ = GlobalContext::Create("x86_64", "haswell");

    // Let all instructions use 1 uop.
    for (int I = 0; I < 10; ++I) {
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(1);
      Decomposition->Uops[0].ProcResIdx = 0;
      llvm::MCInst Inst;
      Inst.setOpcode(I);
      Context_->SetInstructionDecomposition(Inst, std::move(Decomposition));
    }
  }

  static void TearDownTestSuite() { Context_.reset(); }

  static std::unique_ptr<const GlobalContext> Context_;
};

std::unique_ptr<const GlobalContext> RegisterRenamerTest::Context_;

TEST_F(RegisterRenamerTest, TracksRegisters) {
  const auto Tracker = RegisterNameTracker::Create(*Context_->RegisterInfo);

  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre());
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre());
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre());
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre());
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre());

  Tracker->SetName(CX, 1);
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());

  Tracker->SetName(CL, 2);
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(1, 2));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(1, 2));
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(1, 2));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(2));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(1));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());

  Tracker->SetName(CH, 3);
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(2, 3));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(2, 3));
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(2, 3));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(2));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(3));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());

  Tracker->SetName(RCX, 4);
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(4));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(4));
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(4));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(4));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(4));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());

  // Note that in X86, writing to ECX sets upper 32 bits of RCX to 0.
  Tracker->SetName(ECX, 5);
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(5));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(5));
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(5));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(5));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(5));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());

  Tracker->SetName(CX, 6);
  // expected {5,6}
  ASSERT_THAT(Tracker->GetNameDeps(RCX), UnorderedElementsAre(5, 6));
  ASSERT_THAT(Tracker->GetNameDeps(ECX), UnorderedElementsAre(5, 6));
  // ok
  ASSERT_THAT(Tracker->GetNameDeps(CX), UnorderedElementsAre(6));
  ASSERT_THAT(Tracker->GetNameDeps(CL), UnorderedElementsAre(6));
  ASSERT_THAT(Tracker->GetNameDeps(CH), UnorderedElementsAre(6));
  ASSERT_THAT(Tracker->GetNameDeps(EAX), UnorderedElementsAre());
}

TEST_F(RegisterRenamerTest, Renames) {
  RegisterRenamer::Config Config;
  Config.UopsPerCycle = 4;
  Config.NumPhysicalRegisters = 1000;

  TestSource<UopId> Source;
  TestSink<RenamedUopId> Sink;
  RegisterRenamer Renamer(Context_.get(), Config, &Source, &Sink);
  Renamer.Init();

  using llvm::MCInstBuilder;
  std::vector<llvm::MCInst> Instructions;
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32ri).addReg(EAX).addImm(42));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32ri).addReg(EDX).addImm(43));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32ri).addReg(ECX).addImm(44));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::ADD32rr).addReg(EAX).addReg(EAX).addReg(EDX));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV8rr).addReg(DL).addReg(AL));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32rr).addReg(ECX).addReg(EAX));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32rr).addReg(EDI).addReg(ECX));
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV32rr).addReg(ESI).addReg(EDX));
  const BlockContext BlockContext(Instructions, false);

  // Each instruction has one uop.
  Source.Buffer_ = {{TestInstrIndex(0), 0}, {TestInstrIndex(1), 0},
                    {TestInstrIndex(2), 0}, {TestInstrIndex(3), 0},
                    {TestInstrIndex(4), 0}, {TestInstrIndex(5), 0},
                    {TestInstrIndex(6), 0}, {TestInstrIndex(7), 0}};
  Renamer.Tick(&BlockContext);

  // First cycle.
  // Config.UopsPerCycle uops were processed.
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(Field(&RenamedUopId::Type::Uop, EqUopId(0, 0, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 1, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 2, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 3, 0))));

  size_t RenamedEAX;
  {
    const RenamedUopId::Type RenamedUop0 = Sink.Buffer_[0];
    // MOV32ri: -> EAX.
    ASSERT_EQ(RenamedUop0.Uses.size(), 0);
    ASSERT_EQ(RenamedUop0.Defs.size(), 1);
    RenamedEAX = RenamedUop0.Defs[0];
  }

  size_t RenamedEDX;
  {
    // MOV32ri: -> EDX.
    const RenamedUopId::Type RenamedUop1 = Sink.Buffer_[1];
    ASSERT_EQ(RenamedUop1.Uses.size(), 0);
    ASSERT_EQ(RenamedUop1.Defs.size(), 1);
    RenamedEDX = RenamedUop1.Defs[0];
    EXPECT_GT(RenamedEDX, RenamedEAX);
  }

  size_t RenamedECX;
  {
    // MOV32ri: -> ECX.
    const RenamedUopId::Type RenamedUop2 = Sink.Buffer_[2];
    ASSERT_EQ(RenamedUop2.Uses.size(), 0);
    ASSERT_EQ(RenamedUop2.Defs.size(), 1);
    RenamedECX = RenamedUop2.Defs[0];
    EXPECT_GT(RenamedECX, RenamedEDX);
  }

  size_t RenamedEFLAGS;
  {
    // ADD32rr: EAX, EDX -> EAX, (EFLAGS).
    const RenamedUopId::Type RenamedUop3 = Sink.Buffer_[3];
    ASSERT_THAT(RenamedUop3.Uses, ElementsAre(RenamedEAX, RenamedEDX));
    ASSERT_EQ(RenamedUop3.Defs.size(), 2);
    // EAX was renamed twice in a cycle. This is valid.
    RenamedEAX = RenamedUop3.Defs[0];
    EXPECT_GT(RenamedEAX, RenamedECX);
    RenamedEFLAGS = RenamedUop3.Defs[1];
    EXPECT_GT(RenamedEFLAGS, RenamedEAX);
  }

  // Second cycle.
  Sink.Buffer_.clear();
  Renamer.Tick(&BlockContext);
  // Config.UopsPerCycle uops were processed.
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(Field(&RenamedUopId::Type::Uop, EqUopId(0, 4, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 5, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 6, 0)),
                          Field(&RenamedUopId::Type::Uop, EqUopId(0, 7, 0))));

  size_t RenamedDL;
  {
    // MOV8rr AL -> DL
    const RenamedUopId::Type RenamedUop0 = Sink.Buffer_[0];
    ASSERT_THAT(RenamedUop0.Uses, testing::SizeIs(1));
    ASSERT_EQ(RenamedUop0.Defs.size(), 1);
    RenamedDL = RenamedUop0.Defs[0];
    EXPECT_GT(RenamedDL, RenamedEFLAGS);
  }

  {
    // MOV32rr EAX -> ECX
    const RenamedUopId::Type RenamedUop1 = Sink.Buffer_[1];
    ASSERT_THAT(RenamedUop1.Uses, ElementsAre(RenamedEAX));
    ASSERT_EQ(RenamedUop1.Defs.size(), 1);
    RenamedECX = RenamedUop1.Defs[0];
    EXPECT_GT(RenamedECX, RenamedDL);
  }

  size_t RenamedEDI;
  {
    // MOV32rr ECX -> EDI
    const RenamedUopId::Type RenamedUop2 = Sink.Buffer_[2];
    ASSERT_THAT(RenamedUop2.Uses, ElementsAre(RenamedECX));
    ASSERT_EQ(RenamedUop2.Defs.size(), 1);
    RenamedEDI = RenamedUop2.Defs[0];
    EXPECT_GT(RenamedEDI, RenamedECX);
  }

  {
    // MOV32rr EDX -> ESI
    const RenamedUopId::Type RenamedUop3 = Sink.Buffer_[3];
    ASSERT_THAT(RenamedUop3.Uses, UnorderedElementsAre(RenamedEDX, RenamedDL));
    ASSERT_EQ(RenamedUop3.Defs.size(), 1);
    size_t RenamedESI = RenamedUop3.Defs[0];
    EXPECT_GT(RenamedESI, RenamedEDI);
  }
}

TEST_F(RegisterRenamerTest, Handles2OpLea) {
  RegisterRenamer::Config Config;
  Config.UopsPerCycle = 4;
  Config.NumPhysicalRegisters = 1000;

  TestSource<UopId> Source;
  TestSink<RenamedUopId> Sink;
  RegisterRenamer Renamer(Context_.get(), Config, &Source, &Sink);
  Renamer.Init();

  using llvm::MCInstBuilder;
  std::vector<llvm::MCInst> Instructions;
  // lea r13, [r12 + r12 * 0x2]
  Instructions.push_back(
      MCInstBuilder(llvm::X86::MOV64ri).addReg(R12).addImm(42));
  Instructions.push_back(MCInstBuilder(llvm::X86::LEA64r)
                             .addReg(R13)
                             .addReg(R12)
                             .addImm(2)
                             .addReg(R12)
                             .addImm(0)
                             .addReg(0) /* third operand is 0*/);
  const BlockContext BlockContext(Instructions, false);

  Source.Buffer_ = {{TestInstrIndex(0), 0}, {TestInstrIndex(1), 0}};
  Renamer.Tick(&BlockContext);
  ASSERT_EQ(Sink.Buffer_.size(), 2);
  ASSERT_EQ(Sink.Buffer_[0].Defs.size(), 1);  // R12.
  ASSERT_EQ(Sink.Buffer_[1].Uses.size(), 1);  // R12.
  ASSERT_EQ(Sink.Buffer_[1].Defs.size(), 1);  // R13.
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
