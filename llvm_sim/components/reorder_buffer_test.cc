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

#include "llvm_sim/components/reorder_buffer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::AllOf;
using testing::ElementsAre;
using testing::Eq;
using testing::Field;

class ReorderBufferTest : public ::testing::Test {
 protected:
  ReorderBufferTest() {
    // Create a basic block with three instructions.
    llvm::MCInst Inst1;
    llvm::MCInst Inst2;
    llvm::MCInst Inst3;
    llvm::MCInst Inst4;
    llvm::MCInst Inst5;
    Inst1.setOpcode(1);
    Inst2.setOpcode(2);
    Inst3.setOpcode(3);
    Inst4.setOpcode(4);
    Inst5.setOpcode(5);
    Instructions_ = {Inst1, Inst2, Inst3};
    InstrDesc_[1].SchedClass = 1;
    InstrDesc_[2].SchedClass = 2;
    InstrDesc_[3].SchedClass = 3;
    InstrDesc_[4].SchedClass = 4;
    auto InstrInfo = absl::make_unique<llvm::MCInstrInfo>();
    InstrInfo->InitMCInstrInfo(InstrDesc_.data(), nullptr, nullptr,
                               InstrDesc_.size());
    Context_.InstrInfo = std::move(InstrInfo);

    // Setup resources: P0 and P1. TODO(courbet): Add a P01 when supported.
    ProcResources_[1].NumUnits = 1;
    ProcResources_[1].SubUnitsIdxBegin = nullptr;
    ProcResources_[2].NumUnits = 1;
    ProcResources_[2].SubUnitsIdxBegin = nullptr;
    SchedModel_.ProcResourceTable = ProcResources_.data();
    SchedModel_.NumProcResourceKinds = ProcResources_.size();
    llvm::MCSchedClassDesc SchedClassesAnchor;  // Never accessed, but required.
    SchedModel_.SchedClassTable = &SchedClassesAnchor;
    SchedModel_.NumSchedClasses = 0;
    Context_.SchedModel = &SchedModel_;
    Port0Sink_.SetCapacity(1);
    Port1Sink_.SetCapacity(1);

    // Setup fake decompositions.
    {
      // 1 cycle on P0
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(1);
      Decomposition->Uops[0].ProcResIdx = 1;
      Decomposition->Uops[0].StartCycle = 0;
      Decomposition->Uops[0].EndCycle = 1;
      Context_.SetInstructionDecomposition(Inst1, std::move(Decomposition));
    }
    {
      // 1 cycle on P1
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(1);
      Decomposition->Uops[0].ProcResIdx = 2;
      Decomposition->Uops[0].StartCycle = 0;
      Decomposition->Uops[0].EndCycle = 1;
      Context_.SetInstructionDecomposition(Inst2, std::move(Decomposition));
    }
    {
      // 1 cycle on P1 + 2 cycles on P0
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(3);
      Decomposition->Uops[0].ProcResIdx = 2;
      Decomposition->Uops[0].StartCycle = 0;
      Decomposition->Uops[0].EndCycle = 1;
      Decomposition->Uops[1].ProcResIdx = 1;
      Decomposition->Uops[1].StartCycle = 0;
      Decomposition->Uops[1].EndCycle = 1;
      Decomposition->Uops[2].ProcResIdx = 1;
      Decomposition->Uops[2].StartCycle = 0;
      Decomposition->Uops[2].EndCycle = 1;
      Context_.SetInstructionDecomposition(Inst3, std::move(Decomposition));
    }
    {
      // Resourceless Uop
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(1);
      Decomposition->Uops[0].ProcResIdx = 0;
      Decomposition->Uops[0].StartCycle = 0;
      Decomposition->Uops[0].EndCycle = 1;
      Context_.SetInstructionDecomposition(Inst4, std::move(Decomposition));
    }
    {
      // 1 cycle on P1, latency 3 (e.g. IMUL).
      auto Decomposition = absl::make_unique<InstrUopDecomposition>();
      Decomposition->Uops.resize(1);
      Decomposition->Uops[0].ProcResIdx = 2;
      Decomposition->Uops[0].StartCycle = 0;
      Decomposition->Uops[0].EndCycle = 3;
      Context_.SetInstructionDecomposition(Inst5, std::move(Decomposition));
    }

    ReorderBuffer::Config Config;
    Config.NumROBEntries = kTestNumROBEntries;
    ROB_ = absl::make_unique<ReorderBuffer>(
        &Context_, Config, &UopSource_, &AvailableDepsSource_,
        &WritebackSource_, &RetiredSource_, &IssuedSink_,
        std::vector<Sink<ROBUopId>*>{&Port0Sink_, &Port1Sink_},
        &RetirementSink_, IssuePolicy::Greedy());
  }

  static constexpr const int kTestNumROBEntries = 5;

  std::array<llvm::MCInstrDesc, 5> InstrDesc_;
  std::array<llvm::MCProcResourceDesc, 3> ProcResources_;
  llvm::MCSchedModel SchedModel_;
  std::vector<llvm::MCInst> Instructions_;
  GlobalContext Context_;

  TestSource<RenamedUopId> UopSource_;
  TestSource<ROBUopId> WritebackSource_;
  TestSource<ROBUopId> RetiredSource_;
  TestSink<ROBUopId> RetirementSink_;
  TestSink<ROBUopId> IssuedSink_;
  TestSource<ROBUopId> AvailableDepsSource_;
  TestSink<ROBUopId> Port0Sink_;
  TestSink<ROBUopId> Port1Sink_;

  std::unique_ptr<ReorderBuffer> ROB_;
};

const auto HasROBEntryIndex = [](size_t EntryIndex) {
  return Field(&ROBUopId::Type::ROBEntryIndex, Eq(EntryIndex));
};

MATCHER_P2(EqROBUopId, ROBEntryIndex, Uop, "") {
  if (arg.ROBEntryIndex == ROBEntryIndex &&
      arg.Uop.InstrIndex.BBIndex == Uop.InstrIndex.BBIndex &&
      arg.Uop.InstrIndex.Iteration == Uop.InstrIndex.Iteration &&
      arg.Uop.UopIndex == Uop.UopIndex) {
    return true;
  }
  *result_listener << "entry " << arg.ROBEntryIndex << ", uop id ("
                   << arg.Uop.InstrIndex.Iteration << ", "
                   << arg.Uop.InstrIndex.BBIndex << ", " << arg.Uop.UopIndex
                   << ")";
  return false;
}

// Tests that the ROB does not overfill.
TEST_F(ReorderBufferTest, FullROB) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(1, 0).Build();
  UopSource_.Buffer_ = {Uop0, Uop0, Uop0, Uop0, Uop0, Uop1};
  // The unit grabs the elements from the source until the ROB is full.
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_,
              ElementsAre(Field(&RenamedUopId::Type::Uop, EqUopId(0, 1, 0))));

  // Nothing was retired, no free spots to fill.
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_,
              ElementsAre(Field(&RenamedUopId::Type::Uop, EqUopId(0, 1, 0))));
}

// Tests that we cannot dispatch twice to the same port in a cycle.
TEST_F(ReorderBufferTest, P0Conflict) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  UopSource_.Buffer_ = {Uop0, Uop0};  // Both on P0, no deps.
  // The ROB won't get full, but it will take two cycles to dispatch.
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  // The first uop has been dispatched on P0.
  EXPECT_EQ(Port0Sink_.Buffer_.size(), 1);
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 0);
  Port0Sink_.Buffer_.clear();

  ROB_->Tick(&BlockContext);
  // The second uop has been dispatched on P0.
  EXPECT_EQ(Port0Sink_.Buffer_.size(), 1);
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 0);
}

// Tests that we can dispatch to two different ports in a cycle.
TEST_F(ReorderBufferTest, NoConflicts) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(1, 0).Build();
  UopSource_.Buffer_ = {Uop0, Uop1};  // One on P0, one on P1, no deps.
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  EXPECT_EQ(Port0Sink_.Buffer_.size(), 1);
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 1);
  // Check that latencies were populated.
  EXPECT_THAT(Port0Sink_.Buffer_,
              ElementsAre(Field(&ROBUopId::Type::Latency, Eq(1))));
}

// Tests that writeback and sending for retirement work.
TEST_F(ReorderBufferTest, WritebackAndSendingForRetirement) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const UopId::Type Uop = {TestInstrIndex(0), 0};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop).Build()};
  // Issue the uop to P0. We need this because we verify that only kIssued
  // entries can be written back.
  ROB_->Tick(&BlockContext);

  // During this cycle, the entry goes from kIssued to kSentForRetirement.
  const auto ROBUop0 = ROBUopIdBuilder().WithUop(Uop).WithEntryIndex(0).Build();
  AvailableDepsSource_.Buffer_ = {ROBUop0};
  WritebackSource_.Buffer_ = {ROBUop0};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(EqROBUopId(0, Uop)));
}

// Tests that retirement is in order.
TEST_F(ReorderBufferTest, RetirementIsInOrder) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  // We send two uops.
  const UopId::Type Uop0 = {TestInstrIndex(0), 0};
  const UopId::Type Uop1 = {TestInstrIndex(0), 0};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop0).Build(),
                        RenamedUopIdBuilder().WithUop(Uop1).Build(),
                        RenamedUopIdBuilder().WithUop(Uop0).Build()};
  // Issue all uops (for state consistency checking).
  Port0Sink_.SetInfiniteCapacity();
  ROB_->Tick(&BlockContext);
  // The first and third uops are done executing, the third cannot retire
  // because the second one is not, so it stays in kReadyToRetire state.
  const auto ROBUop0 =
      ROBUopIdBuilder().WithUop(Uop0).WithEntryIndex(0).Build();
  const auto ROBUop1 =
      ROBUopIdBuilder().WithUop(Uop1).WithEntryIndex(1).Build();
  const auto ROBUop2 =
      ROBUopIdBuilder().WithUop(Uop0).WithEntryIndex(2).Build();
  AvailableDepsSource_.Buffer_ = {ROBUop0, ROBUop2};
  WritebackSource_.Buffer_ = {ROBUop0, ROBUop2};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));
  RetirementSink_.Buffer_.clear();

  // Now the second is done executing, so the second and third can retire.
  AvailableDepsSource_.Buffer_ = {ROBUop1};
  WritebackSource_.Buffer_ = {ROBUop1};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(RetirementSink_.Buffer_,
              ElementsAre(HasROBEntryIndex(1), HasROBEntryIndex(2)));
}

// Tests that data dependencies on already retired uops (data is in the register
// file) are resolved immediately.
TEST_F(ReorderBufferTest, DataDependenciesOnRegisterFile) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(0, 0).Build(),
                        RenamedUopIdBuilder()
                            .WithUop(1, 0)
                            .AddUse(42)
                            .Build()};  // One on P0, one on P1
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  EXPECT_EQ(Port0Sink_.Buffer_.size(), 1);
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 1);
}

// Tests that data dependencies resolve correctly.
TEST_F(ReorderBufferTest, DataDependenciesOnExecuting) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const UopId::Type Uop0 = {TestInstrIndex(0), 0};
  const UopId::Type Uop1 = {TestInstrIndex(1), 0};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop0).AddDef(42).Build(),
                        RenamedUopIdBuilder().WithUop(Uop1).AddUse(42).Build()};
  ROB_->Tick(&BlockContext);
  // Everything was consumed.
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  // Uop0 started executing.
  EXPECT_EQ(Port0Sink_.Buffer_.size(), 1);
  // But Uop1 has to wait for the result of Uop0.
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 0);
  EXPECT_EQ(IssuedSink_.Buffer_.size(), 1);

  // Uop0 outputs become available.
  AvailableDepsSource_.Buffer_ = {
      ROBUopIdBuilder().WithUop(Uop0).WithEntryIndex(0).Build()};
  ROB_->Tick(&BlockContext);
  // Uop1 started executing.
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 1);
  EXPECT_EQ(IssuedSink_.Buffer_.size(), 2);
}

// Tests that data dependencies on already executed uops (data is in the ROB
// entry) are resolved immediately.
TEST_F(ReorderBufferTest, DataDependenciesOnExecutedBytNotRetired) {
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const UopId::Type Uop0 = {TestInstrIndex(0), 0};
  const UopId::Type Uop1 = {TestInstrIndex(1), 0};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop0).AddDef(42).Build()};
  ROB_->Tick(&BlockContext);
  // Uop0 started executing.
  ASSERT_EQ(Port0Sink_.Buffer_.size(), 1);
  // But Uop1 has to wait for the result of Uop0.
  ASSERT_EQ(Port1Sink_.Buffer_.size(), 0);
  EXPECT_EQ(IssuedSink_.Buffer_.size(), 1);

  // Uop0 outputs become available, Uop1 is consumed.
  AvailableDepsSource_.Buffer_ = {
      ROBUopIdBuilder().WithUop(Uop0).WithEntryIndex(0).Build()};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop1).AddUse(42).Build()};
  ROB_->Tick(&BlockContext);
  // Uop1 started executing.
  EXPECT_EQ(Port1Sink_.Buffer_.size(), 1);
  EXPECT_EQ(IssuedSink_.Buffer_.size(), 2);
}

// Tests that we correctly retire.
TEST_F(ReorderBufferTest, Retirement) {
  ReorderBuffer::Config Config;
  Config.NumROBEntries = 1;
  const auto ROB = absl::make_unique<ReorderBuffer>(
      &Context_, Config, &UopSource_, &AvailableDepsSource_, &WritebackSource_,
      &RetiredSource_, &IssuedSink_,
      std::vector<Sink<ROBUopId>*>{&Port0Sink_, &Port1Sink_}, &RetirementSink_,
      IssuePolicy::Greedy());

  const BlockContext BlockContext(Instructions_, false);
  const UopId::Type Uop0 = {TestInstrIndex(0), 0};
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop0).Build()};
  // Issue the uop.
  Port0Sink_.SetInfiniteCapacity();
  ROB_->Tick(&BlockContext);
  // Write it back.
  const auto ROBUop0 =
      ROBUopIdBuilder().WithUop(Uop0).WithEntryIndex(0).Build();
  AvailableDepsSource_.Buffer_ = {ROBUop0};
  WritebackSource_.Buffer_ = {ROBUop0};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));
  RetirementSink_.Buffer_.clear();
  // Retire it.
  RetiredSource_.Buffer_ = {ROBUop0};
  ROB_->Tick(&BlockContext);

  // Check that the slot was released (the next uop is consumed).
  UopSource_.Buffer_ = {RenamedUopIdBuilder().WithUop(Uop0).Build()};
  ROB_->Tick(&BlockContext);
  EXPECT_TRUE(UopSource_.Buffer_.empty());
}

// Tests that the ROB handles resourceless uops.
TEST_F(ReorderBufferTest, ResourcelessUops) {
  llvm::MCInst Inst;
  Inst.setOpcode(4);
  Instructions_ = {Inst, Inst};
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).AddDef(42).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(1, 0).AddUse(42).Build();
  UopSource_.Buffer_ = {Uop0, Uop1};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());

  // We check that nothing was issued.
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre());
  EXPECT_THAT(Port1Sink_.Buffer_, ElementsAre());
  EXPECT_THAT(IssuedSink_.Buffer_, ElementsAre());
  // But that both uops were sent for retirement.
  EXPECT_THAT(RetirementSink_.Buffer_,
              ElementsAre(HasROBEntryIndex(0), HasROBEntryIndex(1)));
}

// Tests that the ROB handles adding a uops that depends on an already executed
// one within an instruction.
TEST_F(ReorderBufferTest, IntraInstructionDependency) {
  llvm::MCInst Inst;
  Inst.setOpcode(3);
  Instructions_ = {Inst};
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(0, 1).Build();

  // Push and issue Uop0.
  UopSource_.Buffer_ = {Uop0};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  EXPECT_THAT(Port1Sink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));
  EXPECT_THAT(IssuedSink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));

  // Uop0 outputs become available.
  AvailableDepsSource_.Buffer_ = {IssuedSink_.Buffer_[0]};
  ROB_->Tick(&BlockContext);

  // Now push Uop1: It should not get any dep on Uop0 since its output is
  // available, and should execute immediately.
  UopSource_.Buffer_ = {Uop1};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre(HasROBEntryIndex(1)));
  EXPECT_THAT(IssuedSink_.Buffer_,
              ElementsAre(HasROBEntryIndex(0), HasROBEntryIndex(1)));

  // Write Uop0 back: Uop0 should be sent for retirement.
  WritebackSource_.Buffer_ = {IssuedSink_.Buffer_[0]};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(WritebackSource_.Buffer_, ElementsAre());
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));

  // Write Uop1 back and retire Uop0: Uop1 should be sent for retirement.
  AvailableDepsSource_.Buffer_ = {IssuedSink_.Buffer_[1]};
  WritebackSource_.Buffer_ = {IssuedSink_.Buffer_[1]};
  RetiredSource_.Buffer_ = {IssuedSink_.Buffer_[0]};
  RetirementSink_.Buffer_.clear();
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(AvailableDepsSource_.Buffer_, ElementsAre());
  EXPECT_THAT(WritebackSource_.Buffer_, ElementsAre());
  EXPECT_THAT(RetiredSource_.Buffer_, ElementsAre());
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(HasROBEntryIndex(1)));
}

// Tests that the ROB correctly fetches latencies.
TEST_F(ReorderBufferTest, UsesLatencies) {
  llvm::MCInst Inst1;
  Inst1.setOpcode(1);
  llvm::MCInst Inst2;
  Inst2.setOpcode(5);
  Instructions_ = {Inst1, Inst2};
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(1, 0).Build();
  UopSource_.Buffer_ = {Uop0, Uop1};
  ROB_->Tick(&BlockContext);
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());

  EXPECT_THAT(Port0Sink_.Buffer_,
              ElementsAre(AllOf(HasROBEntryIndex(0),
                                Field(&ROBUopId::Type::Latency, Eq(1)))));
  EXPECT_THAT(Port1Sink_.Buffer_,
              ElementsAre(AllOf(HasROBEntryIndex(1),
                                Field(&ROBUopId::Type::Latency, Eq(3)))));
}

// Tests that oldest entries are processed first.
TEST_F(ReorderBufferTest, OldestFirst) {
  ReorderBuffer::Config Config;
  Config.NumROBEntries = 2;
  const auto ROB = absl::make_unique<ReorderBuffer>(
      &Context_, Config, &UopSource_, &AvailableDepsSource_, &WritebackSource_,
      &RetiredSource_, &IssuedSink_,
      std::vector<Sink<ROBUopId>*>{&Port0Sink_, &Port1Sink_}, &RetirementSink_,
      IssuePolicy::Greedy());

  llvm::MCInst Inst;
  Inst.setOpcode(1);
  Instructions_ = {Inst, Inst, Inst};
  const BlockContext BlockContext(Instructions_, false);
  ROB->Init();

  const auto Uop0 = RenamedUopIdBuilder().WithUop(0, 0).Build();
  const auto Uop1 = RenamedUopIdBuilder().WithUop(1, 0).Build();
  const auto Uop2 = RenamedUopIdBuilder().WithUop(2, 0).Build();
  UopSource_.Buffer_ = {Uop0, Uop1, Uop2};
  ROB->Tick(&BlockContext);
  ASSERT_THAT(Port0Sink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));

  // Fully execute and retire Uop0.
  AvailableDepsSource_.Buffer_ = {Port0Sink_.Buffer_[0]};
  WritebackSource_.Buffer_ = {Port0Sink_.Buffer_[0]};
  ROB->Tick(&BlockContext);
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));
  EXPECT_THAT(RetirementSink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));
  RetiredSource_.Buffer_ = {Port0Sink_.Buffer_[0]};
  ROB->Tick(&BlockContext);
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre(HasROBEntryIndex(0)));

  // Because there are only two ROB entries, Uop2 will end up in front in the
  // circular buffer. Make sure Uop1 is the one that gets issued first.
  Port0Sink_.Buffer_.clear();
  ROB->Tick(&BlockContext);
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre(HasROBEntryIndex(1)));
}

// Tests that the ROB can deal with a uop depending on another uop through more
// than one register.
TEST_F(ReorderBufferTest, MultipleDependencies) {
  llvm::MCInst Inst;
  Inst.setOpcode(4);
  Instructions_ = {Inst, Inst};
  const BlockContext BlockContext(Instructions_, false);
  ROB_->Init();

  const auto Uop0 = RenamedUopIdBuilder()
                        .WithUop(0, 0)
                        .AddDef(42)
                        .AddDef(43)
                        .AddUse(42)
                        .AddUse(43);
  const auto Uop1 = RenamedUopIdBuilder(Uop0).WithUop(1, 0);
  UopSource_.Buffer_ = {Uop0.Build(), Uop1.Build()};
  ROB_->Tick(&BlockContext);
  // Without duplicate dependency handling Tick crashes. Otherwise test these
  // were resourceless uops.
  EXPECT_THAT(UopSource_.Buffer_, ElementsAre());
  EXPECT_THAT(Port0Sink_.Buffer_, ElementsAre());
  EXPECT_THAT(Port1Sink_.Buffer_, ElementsAre());
  EXPECT_THAT(IssuedSink_.Buffer_, ElementsAre());
  EXPECT_THAT(RetirementSink_.Buffer_,
              ElementsAre(HasROBEntryIndex(0), HasROBEntryIndex(1)));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
