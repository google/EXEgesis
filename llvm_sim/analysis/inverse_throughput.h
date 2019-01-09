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

#ifndef EXEGESIS_LLVM_SIM_ANALYSIS_INVERSE_THROUGHPUT_H_
#define EXEGESIS_LLVM_SIM_ANALYSIS_INVERSE_THROUGHPUT_H_

#include <limits>
#include <vector>

#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {

// We always compute min-max throughtput because it can vary from iteration to
// iteration and we'd rather give a range than a fractional number.
struct InverseThroughputAnalysis {
  unsigned Min = std::numeric_limits<unsigned>::max();
  unsigned Max = std::numeric_limits<unsigned>::min();
  // The estimation is based on the last `NumIterations` iterations. This
  // gives a more precise result by allowing to reach steady state.
  unsigned NumIterations = 0;
  // The total number of cycles used by the simulation over all iterations. This
  // gives a better idea of the average inverse throughput than the min/max.
  unsigned TotalNumCycles = 0;
};

// Computes the inverse throughput.
InverseThroughputAnalysis ComputeInverseThroughput(
    const BlockContext& BlockContext, const SimulationLog& Log);

// Extracts the number of cycles that each iteration of the basic block has
// taken. Drops a small number of iterations at the beginning of the simulation
// so that only iterations where the pipeline was properly warmed up are
// considered.
std::vector<unsigned> ComputeInverseThroughputs(
    const BlockContext& BlockContext, const SimulationLog& Log);

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_ANALYSIS_INVERSE_THROUGHPUT_H_
