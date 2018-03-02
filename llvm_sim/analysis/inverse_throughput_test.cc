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
  }
  Log.Iterations.push_back({/*EndCycle=*/2});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 2);
    EXPECT_EQ(Result.Max, 2);
    EXPECT_EQ(Result.NumIterations, 1);
  }
  Log.Iterations.push_back({/*EndCycle=*/15});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 13);  // Skipped first iteration.
    EXPECT_EQ(Result.Max, 13);
    EXPECT_EQ(Result.NumIterations, 1);
  }
  Log.Iterations.push_back({/*EndCycle=*/42});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 13);  // Skipped first iteration.
    EXPECT_EQ(Result.Max, 27);
    EXPECT_EQ(Result.NumIterations, 2);
  }
  Log.Iterations.push_back({/*EndCycle=*/44});
  {
    const InverseThroughputAnalysis Result =
        ComputeInverseThroughput(BlockContext, Log);
    EXPECT_EQ(Result.Min, 2);  // Skipped first two iterations.
    EXPECT_EQ(Result.Max, 27);
    EXPECT_EQ(Result.NumIterations, 2);
  }
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
