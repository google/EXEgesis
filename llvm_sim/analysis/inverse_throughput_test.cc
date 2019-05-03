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

#include "llvm_sim/analysis/inverse_throughput.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {
namespace {

TEST(DispatchPortTest, Works) {
  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, true);

  std::vector<BufferDescription> BufferDescriptions(4);

  SimulationLog Log(BufferDescriptions);

  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, std::numeric_limits<unsigned>::max());
    EXPECT_EQ(Result.Max, std::numeric_limits<unsigned>::min());
    EXPECT_EQ(Result.NumIterations, 0);
    EXPECT_EQ(Result.TotalNumCycles, 0);
  }
  Log.Iterations.push_back({/*EndCycle=*/2});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 2);
    EXPECT_EQ(Result.Max, 2);
    EXPECT_EQ(Result.NumIterations, 1);
    EXPECT_EQ(Result.TotalNumCycles, 2);
  }
  Log.Iterations.push_back({/*EndCycle=*/15});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 13);  // Skipped first iteration.
    EXPECT_EQ(Result.Max, 13);
    EXPECT_EQ(Result.NumIterations, 1);
    EXPECT_EQ(Result.TotalNumCycles, 13);
  }
  Log.Iterations.push_back({/*EndCycle=*/42});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 13);  // Skipped first iteration.
    EXPECT_EQ(Result.Max, 27);
    EXPECT_EQ(Result.NumIterations, 2);
    EXPECT_EQ(Result.TotalNumCycles, 40);
  }
  Log.Iterations.push_back({/*EndCycle=*/44});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 2);  // Skipped first two iterations.
    EXPECT_EQ(Result.Max, 27);
    EXPECT_EQ(Result.NumIterations, 2);
    EXPECT_EQ(Result.TotalNumCycles, 29);
  }
}

TEST(ComputeInverseThroughputsTest, SanityCheck) {
  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, true);

  std::vector<BufferDescription> BufferDescriptions(4);
  SimulationLog Log(BufferDescriptions);
  Log.Iterations = {{/*EndCycle=*/4}, {/*EndCycle=*/5},  {/*EndCycle=*/7},
                    {/*EndCycle=*/9}, {/*EndCycle=*/11}, {/*EndCycle=*/13}};
  const std::vector<unsigned> Throughputs =
      ComputeInverseThroughputs(BlockContext, Log);
  EXPECT_THAT(Throughputs, ::testing::ElementsAre(2, 2, 2));
}

// Checks that at most 5 items are skipped when enough data is available.
TEST(ComputeInverseThroughputsTest, SkippedItems) {
  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, true);

  std::vector<BufferDescription> BufferDescriptions(4);
  SimulationLog Log(BufferDescriptions);
  Log.Iterations = {{/*EndCycle=*/4},  {/*EndCycle=*/5},  {/*EndCycle=*/7},
                    {/*EndCycle=*/9},  {/*EndCycle=*/11}, {/*EndCycle=*/13},
                    {/*EndCycle=*/17}, {/*EndCycle=*/19}, {/*EndCycle=*/22},
                    {/*EndCycle=*/23}, {/*EndCycle=*/25}, {/*EndCycle=*/28}};
  const std::vector<unsigned> Throughputs =
      ComputeInverseThroughputs(BlockContext, Log);
  EXPECT_THAT(Throughputs, ::testing::ElementsAre(2, 4, 2, 3, 1, 2, 3));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
