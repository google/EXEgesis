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

namespace exegesis {
namespace simulator {

// Computes the inverse throughput.
InverseThroughputAnalysis ComputeInverseThroughput(
    const BlockContext& BlockContext, const SimulationLog& Log) {
  InverseThroughputAnalysis Result;
  // To compute the throughput, we want to be in in steady state. We skip the
  // first half of the iterations when we can.
  const size_t SkippedIterations = Log.GetNumCompleteIterations() / 2;
  unsigned PrevEndCycle = SkippedIterations == 0
                              ? 0
                              : Log.Iterations[SkippedIterations - 1].EndCycle;
  Result.NumIterations = Log.GetNumCompleteIterations() - SkippedIterations;
  for (int I = SkippedIterations; I < Log.GetNumCompleteIterations(); ++I) {
    const unsigned ThisEndCycle = Log.Iterations[I].EndCycle;
    assert(ThisEndCycle >= PrevEndCycle && "not in order");
    const unsigned InvThroughput = ThisEndCycle - PrevEndCycle;
    Result.Min = std::min(Result.Min, InvThroughput);
    Result.Max = std::max(Result.Max, InvThroughput);
    PrevEndCycle = ThisEndCycle;
  }
  return Result;
}

}  // namespace simulator
}  // namespace exegesis
