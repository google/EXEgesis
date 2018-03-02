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
  auto InstrInfo = llvm::make_unique<llvm::MCInstrInfo>();
  std::array<llvm::MCInstrDesc, 4> InstrDesc;
  InstrDesc[1].SchedClass = 1;
  InstrDesc[2].SchedClass = 2;
  InstrDesc[3].SchedClass = 3;
  InstrInfo->InitMCInstrInfo(InstrDesc.data(), /*InstrNameIndices=*/nullptr,
                             /*InstrNameData=*/nullptr, InstrDesc.size());
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
  std::array<llvm::MCSchedClassDesc, 4> SchedClasses;
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
  SchedModel.SchedClassTable = SchedClasses.data();
  SchedModel.NumSchedClasses = SchedClasses.size();
  SchedModel.InstrItineraries = nullptr;
  Context.SchedModel = &SchedModel;

  // Setup SubtargetInfo
  std::array<llvm::MCWriteProcResEntry, 8> WriteProcResEntries;
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
  // Setup write latency tables.
  std::array<llvm::MCWriteLatencyEntry, 2> WriteLatencyEntries;
  // Latency 4 on first def.
  WriteLatencyEntries[1].Cycles = 4;
  WriteLatencyEntries[1].WriteResourceID = 0;
  Context.SubtargetInfo = llvm::make_unique<llvm::MCSubtargetInfo>(
      llvm::Triple(), /*CPU=*/"", /*FeatureString=*/"",
      llvm::ArrayRef<llvm::SubtargetFeatureKV>(),
      llvm::ArrayRef<llvm::SubtargetFeatureKV>(), /*ProcSchedModels=*/nullptr,
      WriteProcResEntries.data(), WriteLatencyEntries.data(),
      /*ReadAdvanceEntries=*/nullptr, /*InstrStages=*/nullptr,
      /*OperandCycles=*/nullptr, /*ForwardingPaths=*/nullptr);

  ASSERT_EQ(Context.GetSchedClassForInstruction(1), &SchedClasses[1]);
  ASSERT_EQ(Context.GetSchedClassForInstruction(2), &SchedClasses[2]);
  ASSERT_EQ(Context.GetSchedClassForInstruction(3), &SchedClasses[3]);

  const auto ProcResIdxIs = [](const unsigned ProcResIdx) {
    return Field(&InstrUopDecomposition::Uop::ProcResIdx, Eq(ProcResIdx));
  };
  const auto StartEndCyclesAre = [](const unsigned Start, const unsigned End) {
    return AllOf(Field(&InstrUopDecomposition::Uop::StartCycle, Eq(Start)),
                 Field(&InstrUopDecomposition::Uop::EndCycle, Eq(End)));
  };
  {
    const auto& Decomposition = Context.GetInstructionDecomposition(1);
    ASSERT_THAT(Decomposition.Uops, ElementsAre(ProcResIdxIs(1)));
    ASSERT_THAT(Decomposition.Uops, ElementsAre(StartEndCyclesAre(0, 1)));
  }
  {
    const auto& Decomposition = Context.GetInstructionDecomposition(2);
    ASSERT_THAT(Decomposition.Uops, ElementsAre(ProcResIdxIs(2)));
    ASSERT_THAT(Decomposition.Uops, ElementsAre(StartEndCyclesAre(0, 1)));
  }
  {
    const auto& Decomposition = Context.GetInstructionDecomposition(3);
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(ProcResIdxIs(1), ProcResIdxIs(1), ProcResIdxIs(2)));
    ASSERT_THAT(Decomposition.Uops,
                ElementsAre(StartEndCyclesAre(0, 2), StartEndCyclesAre(2, 3),
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

}  // namespace
}  // namespace simulator
}  // namespace exegesis
