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

#include "llvm_sim/framework/context.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::AllOf;
using testing::ElementsAre;
using testing::Eq;
using testing::Field;

TEST(ContextTest, Decomposition) {
  GlobalContext Context;

  // Setup fake InstrInfo.
  auto InstrInfo = absl::make_unique<llvm::MCInstrInfo>();
  constexpr char InstrNameData[] = "I0\0I1\0I2\0I3\0I4";
  constexpr std::array<unsigned, 5> InstrNameIndices = {0, 3, 6, 9, 12};
  std::array<llvm::MCInstrDesc, 5> InstrDesc;
  static_assert(InstrDesc.size() == InstrNameIndices.size(),
                "The number of instruction names does not match the number of "
                "instructions");
  InstrDesc[1].SchedClass = 1;
  InstrDesc[2].SchedClass = 2;
  InstrDesc[3].SchedClass = 3;
  InstrDesc[4].SchedClass = 4;
  InstrInfo->InitMCInstrInfo(InstrDesc.data(), InstrNameIndices.data(),
                             InstrNameData, InstrDesc.size());
  Context.InstrInfo = std::move(InstrInfo);

  llvm::MCSchedModel SchedModel;

  // Setup proc resources.
  std::array<llvm::MCProcResourceDesc, 4> ProcResources;
  // P0.
  ProcResources[1].NumUnits = 1;
  ProcResources[1].SubUnitsIdxBegin = nullptr;
  // P1.
  ProcResources[2].NumUnits = 1;
  ProcResources[2].SubUnitsIdxBegin = nullptr;
  // P01.
  ProcResources[3].NumUnits = 2;
  unsigned P01Subunits[] = {1, 2};
  ProcResources[3].SubUnitsIdxBegin = P01Subunits;
  SchedModel.ProcResourceTable = ProcResources.data();
  SchedModel.NumProcResourceKinds = ProcResources.size();

  // Setup Sched Classes.
  std::array<llvm::MCSchedClassDesc, 5> SchedClasses;
  // Class 1: 1 cycle on P0, denormalized to {1xP0, 1xP01}.
  SchedClasses[1].NumMicroOps = 1;
  SchedClasses[1].WriteProcResIdx = 1;
  SchedClasses[1].NumWriteProcResEntries = 2;
  SchedClasses[1].WriteLatencyIdx = 0;
  SchedClasses[1].NumWriteLatencyEntries = 0;
  // Class 2: 1 cycle on P1, denormalized to {1xP1, 1xP01}
  SchedClasses[2].NumMicroOps = 1;
  SchedClasses[2].WriteProcResIdx = 3;
  SchedClasses[2].NumWriteProcResEntries = 2;
  SchedClasses[2].WriteLatencyIdx = 0;
  SchedClasses[2].NumWriteLatencyEntries = 0;
  // Class 3: 1 cycle on P1, 2 cycles on P0, denormalized to {2xP0, 1xP1, 3xP01}
  SchedClasses[3].NumMicroOps = 3;
  SchedClasses[3].WriteProcResIdx = 5;
  SchedClasses[3].NumWriteProcResEntries = 3;
  SchedClasses[3].WriteLatencyIdx = 1;
  SchedClasses[3].NumWriteLatencyEntries = 1;
  // Class 4: 2 cycles on P0, 3 cycles on P1, denormalized to
  // {2xP0, 3xP1, 5xP01}.
  SchedClasses[4].NumMicroOps = 5;
  SchedClasses[4].WriteProcResIdx = 8;
  SchedClasses[4].NumWriteProcResEntries = 3;
  SchedClasses[4].WriteLatencyIdx = 1;
  SchedClasses[4].NumWriteLatencyEntries = 1;
  SchedModel.SchedClassTable = SchedClasses.data();
  SchedModel.NumSchedClasses = SchedClasses.size();
  SchedModel.InstrItineraries = nullptr;
  Context.SchedModel = &SchedModel;

  // Setup SubtargetInfo
  std::array<llvm::MCWriteProcResEntry, 11> WriteProcResEntries;
  // {1xP0, 1xP01}
  WriteProcResEntries[1].ProcResourceIdx = 1;
  WriteProcResEntries[1].Cycles = 1;
  WriteProcResEntries[2].ProcResourceIdx = 3;
  WriteProcResEntries[2].Cycles = 1;
  // {1xP1, 1xP01}
  WriteProcResEntries[3].ProcResourceIdx = 2;
  WriteProcResEntries[3].Cycles = 1;
  WriteProcResEntries[4].ProcResourceIdx = 3;
  WriteProcResEntries[4].Cycles = 1;
  // {2xP0, 1xP1, 3xP01}
  WriteProcResEntries[5].ProcResourceIdx = 1;
  WriteProcResEntries[5].Cycles = 2;
  WriteProcResEntries[6].ProcResourceIdx = 2;
  WriteProcResEntries[6].Cycles = 1;
  WriteProcResEntries[7].ProcResourceIdx = 3;
  WriteProcResEntries[7].Cycles = 3;
  // {2xP0, 3xP1, 5xP01}
  WriteProcResEntries[8].ProcResourceIdx = 1;
  WriteProcResEntries[8].Cycles = 2;
  WriteProcResEntries[9].ProcResourceIdx = 2;
  WriteProcResEntries[9].Cycles = 3;
  WriteProcResEntries[10].ProcResourceIdx = 3;
  WriteProcResEntries[10].Cycles = 5;
  // Setup write latency tables.
  std::array<llvm::MCWriteLatencyEntry, 2> WriteLatencyEntries;
  // Latency 4 on first def.
  WriteLatencyEntries[1].Cycles = 4;
  WriteLatencyEntries[1].WriteResourceID = 0;
  Context.SubtargetInfo = absl::make_unique<llvm::MCSubtargetInfo>(
      llvm::Triple(), /*CPU=*/"", /*FeatureString=*/"",
      llvm::ArrayRef<llvm::SubtargetFeatureKV>(),
      llvm::ArrayRef<llvm::SubtargetSubTypeKV>(), WriteProcResEntries.data(),
      WriteLatencyEntries.data(),
      /*ReadAdvanceEntries=*/nullptr, /*InstrStages=*/nullptr,
      /*OperandCycles=*/nullptr, /*ForwardingPaths=*/nullptr);

  llvm::MCInst Inst;
  Inst.setOpcode(1);
  ASSERT_EQ(Context.GetSchedClassForInstruction(Inst), &SchedClasses[1]);
  Inst.setOpcode(2);
  ASSERT_EQ(Context.GetSchedClassForInstruction(Inst), &SchedClasses[2]);
  Inst.setOpcode(3);
  ASSERT_EQ(Context.GetSchedClassForInstruction(Inst), &SchedClasses[3]);
  Inst.setOpcode(4);
  ASSERT_EQ(Context.GetSchedClassForInstruction(Inst), &SchedClasses[4]);

  const auto ProcResIdxIs = [](const unsigned ProcResIdx) {
    return Field(&InstrUopDecomposition::Uop::ProcResIdx, Eq(ProcResIdx));
  };
  const auto StartEndCyclesAre = [](const unsigned Start, const unsigned End) {
    return AllOf(Field(&InstrUopDecomposition::Uop::StartCycle, Eq(Start)),
                 Field(&InstrUopDecomposition::Uop::EndCycle, Eq(End)));
  };
  {
    llvm::MCInst Inst;
    Inst.setOpcode(1);
    const auto& Decomposition = Context.GetInstructionDecomposition(Inst);
    ASSERT_THAT(Decomposition.Uops, ElementsAre(ProcResIdxIs(1)));
    ASSERT_THAT(Decomposition.Uops, ElementsAre(StartEndCyclesAre(0, 1)));
  }
  {
    llvm::MCInst Inst;
    Inst.setOpcode(2);
    const auto& Decomposition = Context.GetInstructionDecomposition(Inst);
    ASSERT_THAT(Decomposition.Uops, ElementsAre(ProcResIdxIs(2)));
    ASSERT_THAT(Decomposition.Uops, ElementsAre(StartEndCyclesAre(0, 1)));
  }
  {
    llvm::MCInst Inst;
    Inst.setOpcode(3);
    const auto& Decomposition = Context.GetInstructionDecomposition(Inst);
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(ProcResIdxIs(1), ProcResIdxIs(1), ProcResIdxIs(2)));
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(StartEndCyclesAre(0, 2), StartEndCyclesAre(2, 3),
                            StartEndCyclesAre(3, 4)));
  }
  {
    llvm::MCInst Inst;
    Inst.setOpcode(4);
    const auto& Decomposition = Context.GetInstructionDecomposition(Inst);
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(ProcResIdxIs(1), ProcResIdxIs(1), ProcResIdxIs(2),
                            ProcResIdxIs(2), ProcResIdxIs(2)));
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(StartEndCyclesAre(0, 1), StartEndCyclesAre(1, 2),
                            StartEndCyclesAre(2, 3), StartEndCyclesAre(3, 4),
                            StartEndCyclesAre(3, 4)));
  }
}

TEST(BlockContextTest, GetInstruction) {
  std::vector<llvm::MCInst> Instructions(2);
  const BlockContext BlockContext(Instructions, false);
  ASSERT_FALSE(BlockContext.IsLoop());
  ASSERT_EQ(BlockContext.GetNumBasicBlockInstructions(), 2);
  ASSERT_NE(&BlockContext.GetInstruction(0), &BlockContext.GetInstruction(1));
}

TEST(BlockContextTest, MCInstEqHash) {
  const GlobalContext::MCInstEq InstEq;
  const absl::Hash<llvm::MCInst> InstHash;

  llvm::MCInst A, B;

  A.setOpcode(123);
  B.setOpcode(456);
  EXPECT_TRUE(InstEq(B, B));
  EXPECT_EQ(InstHash(B), InstHash(B));
  EXPECT_FALSE(InstEq(A, B));

  B.setOpcode(123);
  EXPECT_TRUE(InstEq(A, B));
  EXPECT_EQ(InstHash(A), InstHash(B));
  A.setFlags(2);
  B.setFlags(4);
  EXPECT_TRUE(InstEq(B, B));
  EXPECT_EQ(InstHash(B), InstHash(B));
  EXPECT_FALSE(InstEq(A, B));

  B.setFlags(2);
  EXPECT_TRUE(InstEq(A, B));
  EXPECT_EQ(InstHash(A), InstHash(B));
  A.addOperand(llvm::MCOperand::createReg(12));
  EXPECT_TRUE(InstEq(B, B));
  EXPECT_EQ(InstHash(B), InstHash(B));
  EXPECT_FALSE(InstEq(A, B));

  B.addOperand(llvm::MCOperand::createReg(11));
  EXPECT_FALSE(InstEq(A, B));
  B.getOperand(0).setReg(12);
  EXPECT_TRUE(InstEq(A, B));
  EXPECT_EQ(InstHash(A), InstHash(B));
  A.addOperand(llvm::MCOperand::createImm(12));
  B.addOperand(llvm::MCOperand::createImm(11));
  EXPECT_TRUE(InstEq(B, B));
  EXPECT_EQ(InstHash(B), InstHash(B));
  EXPECT_FALSE(InstEq(A, B));
  B.getOperand(1).setImm(12);
  EXPECT_TRUE(InstEq(A, B));
  EXPECT_EQ(InstHash(A), InstHash(B));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
