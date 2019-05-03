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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;

MATCHER_P2(EqPortPressure, BufferIndex, CyclesPerIteration, "") {
  if (arg.BufferIndex == BufferIndex &&
      arg.CyclesPerIteration == CyclesPerIteration) {
    return true;
  }
  *result_listener << "(BufferIndex=" << arg.BufferIndex
                   << ", CyclesPerIteration=" << arg.CyclesPerIteration << ")";
  return false;
}

TEST(DispatchPortTest, Works) {
  std::vector<llvm::MCInst> Instructions(3);
  const BlockContext BlockContext(Instructions, true);

  std::vector<BufferDescription> BufferDescriptions(4);

  SimulationLog Log(BufferDescriptions);

  // 2 iterations.
  Log.Iterations.push_back({/*EndCycle=*/2});
  Log.Iterations.push_back({/*EndCycle=*/3});

  // Initialization: buffers 0, 1, and 3 have port pressure data. Note that
  // port 0 should be present in the output with pressure 0 even if it never
  // gets used.
  Log.Lines.push_back({/*Cycle=*/0, /*BufferIndex=*/0, "PortPressure", "init"});
  Log.Lines.push_back({/*Cycle=*/0, /*BufferIndex=*/1, "PortPressure", "init"});
  Log.Lines.push_back({/*Cycle=*/0, /*BufferIndex=*/3, "PortPressure", "init"});
  // Add sparse port pressure data at various cycles.
  Log.Lines.push_back(
      {/*Cycle=*/0, /*BufferIndex=*/1, "PortPressure", "0,0,1"});
  Log.Lines.push_back(
      {/*Cycle=*/0, /*BufferIndex=*/3, "PortPressure", "0,0,1"});
  Log.Lines.push_back(
      {/*Cycle=*/1, /*BufferIndex=*/3, "PortPressure", "0,1,0.5"});
  Log.Lines.push_back(
      {/*Cycle=*/2, /*BufferIndex=*/3, "PortPressure", "1,2,0.5"});
  // This is not port pressure data, ignore.
  Log.Lines.push_back({/*Cycle=*/0, /*BufferIndex=*/0, "Ignored", "N/A"});
  Log.Lines.push_back({/*Cycle=*/0, /*BufferIndex=*/2, "Ignored", "N/A"});
  // This is an incomplete iteration, ignore.
  Log.Lines.push_back(
      {/*Cycle=*/2, /*BufferIndex=*/0, "PortPressure", "2,1,1"});

  const PortPressureAnalysis Result = ComputePortPressure(BlockContext, Log);
  EXPECT_THAT(Result.Pressures,
              ElementsAre(EqPortPressure(0, 0.0f), EqPortPressure(1, 0.5f),
                          EqPortPressure(3, 1.0f)));
  // Pressures per instruction for buffer 0.
  EXPECT_THAT(Result.Pressures[0].CyclesPerIterationByMCInst,
              ElementsAre(0.0f, 0.0f, 0.0f));
  // Pressures per instruction for buffer 1.
  EXPECT_THAT(Result.Pressures[1].CyclesPerIterationByMCInst,
              ElementsAre(0.5f, 0.0f, 0.0f));
  // Pressures per instruction for buffer 3.
  EXPECT_THAT(Result.Pressures[2].CyclesPerIterationByMCInst,
              ElementsAre(0.5f, 0.25f, 0.25f));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
