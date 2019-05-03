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

#include <algorithm>

namespace exegesis {
namespace simulator {
namespace {

// A generic function that walks through the iterations of the basic block in
// the simulation log. Skips the first SkippedIterations iterations. Calls
// InverseThroughputCallback for each (non-skipped) iteration and passes the
// number of cycles that the iteration took as the argument.
template <typename CallbackType>
void ProcessInverseThroughputs(const BlockContext& BlockContext,
                               const SimulationLog& Log, int SkippedIterations,
                               const CallbackType& InverseThroughputCallback) {
  const unsigned StartCycle =
      SkippedIterations == 0 ? 0
                             : Log.Iterations[SkippedIterations - 1].EndCycle;
  unsigned PrevEndCycle = StartCycle;
  for (int I = SkippedIterations; I < Log.GetNumCompleteIterations(); ++I) {
    const unsigned ThisEndCycle = Log.Iterations[I].EndCycle;
    assert(ThisEndCycle >= PrevEndCycle && "not in order");
    InverseThroughputCallback(ThisEndCycle - PrevEndCycle);
    PrevEndCycle = ThisEndCycle;
  }
}

}  // namespace

// Computes the inverse throughput.
InverseThroughputAnalysis ComputeInverseThroughput(
    const BlockContext& BlockContext, const SimulationLog& Log) {
  // To compute the throughput, we want to be in a steady state. We skip the
  // first half of the iterations when we can.
  const size_t SkippedIterations = Log.GetNumCompleteIterations() / 2;
  InverseThroughputAnalysis Result;
  Result.NumIterations = Log.GetNumCompleteIterations() - SkippedIterations;
  ProcessInverseThroughputs(BlockContext, Log, SkippedIterations,
                            [&Result](unsigned InvThroughput) {
                              Result.Min = std::min(Result.Min, InvThroughput);
                              Result.Max = std::max(Result.Max, InvThroughput);
                            });
  const unsigned StartCycle =
      SkippedIterations == 0 ? 0
                             : Log.Iterations[SkippedIterations - 1].EndCycle;
  const unsigned EndCycle =
      Log.GetNumCompleteIterations() == 0
          ? 0
          : Log.Iterations[Log.GetNumCompleteIterations() - 1].EndCycle;
  Result.TotalNumCycles = EndCycle - StartCycle;
  return Result;
}

std::vector<unsigned> ComputeInverseThroughputs(
    const BlockContext& BlockContext, const SimulationLog& Log) {
  std::vector<unsigned> Throughputs;
  // To compute the throughput, we want to be in a steady state. We skip the
  // first five iterations when we can; when there is less than 10 iterations,
  // we skip the first half.
  // TODO(ondrasej): Consider other metrics for determining the number of
  // iterations to be skipped.
  const size_t SkippedIterations =
      std::min(Log.GetNumCompleteIterations() / 2, 5UL);
  ProcessInverseThroughputs(BlockContext, Log, SkippedIterations,
                            [&Throughputs](unsigned InvThroughput) {
                              Throughputs.push_back(InvThroughput);
                            });
  return Throughputs;
}

}  // namespace simulator
}  // namespace exegesis
