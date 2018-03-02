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

#include "llvm_sim/analysis/port_pressure.h"

#include "llvm/ADT/Optional.h"

namespace exegesis {
namespace simulator {

PortPressureAnalysis ComputePortPressure(const BlockContext& BlockContext,
                                         const SimulationLog& Log) {
  std::vector<llvm::Optional<std::vector<float>>> TotalCyclesByInstByBuffer(
      Log.BufferDescriptions.size());

  for (const auto& Line : Log.Lines) {
    if (Line.MsgTag != "PortPressure") {
      continue;
    }
    auto& CyclesByInst = TotalCyclesByInstByBuffer[Line.BufferIndex];
    if (Line.Msg == "init") {
      // Initialize the port if needed.
      assert(!CyclesByInst.hasValue() && "initialized twice");
      CyclesByInst =
          std::vector<float>(BlockContext.GetNumBasicBlockInstructions());
    } else {
      // `Msg` is "<iteration>,<inst index>,<pressure_in_cycles>".
      llvm::StringRef Msg = Line.Msg;
      InstructionIndex::Type Instr;
      if (InstructionIndex::Consume(Msg, Instr)) {
        llvm::errs() << Msg << "\n";
        llvm_unreachable("invalid PortPressure message");
      }
      if (Instr.Iteration >= Log.GetNumCompleteIterations()) {
        // Ignore any incomplete iteration to avoid biasing the numbers.
        continue;
      }
      assert(Instr.BBIndex < BlockContext.GetNumBasicBlockInstructions());
      assert(!Msg.empty() && Msg.front() == ',');
      Msg = Msg.drop_front();
      double Cycles;
      if (Msg.getAsDouble(Cycles)) {
        llvm_unreachable("invalid PortPressure message");
      }
      (*CyclesByInst)[Instr.BBIndex] += Cycles;
    }
  }

  // Collect results.
  PortPressureAnalysis Result;
  for (int BufferIdx = 0; BufferIdx < Log.BufferDescriptions.size();
       ++BufferIdx) {
    auto& CyclesByInst = TotalCyclesByInstByBuffer[BufferIdx];
    if (!CyclesByInst.hasValue()) {
      continue;  // Not a port.
    }
    PortPressureAnalysis::PortPressure Pressure;
    Pressure.BufferIndex = BufferIdx;
    for (const float Cycles : *CyclesByInst) {
      Pressure.CyclesPerIterationByMCInst.push_back(
          Cycles / Log.GetNumCompleteIterations());
      Pressure.CyclesPerIteration += Cycles;
    }
    Pressure.CyclesPerIteration /= Log.GetNumCompleteIterations();
    Result.Pressures.push_back(std::move(Pressure));
  }
  return Result;
}

}  // namespace simulator
}  // namespace exegesis
