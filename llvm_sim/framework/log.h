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

// The result of a simulation.
#ifndef EXEGESIS_LLVM_SIM_FRAMEWORK_LOG_H_
#define EXEGESIS_LLVM_SIM_FRAMEWORK_LOG_H_

#include <string>

#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// The description of a buffer.
struct BufferDescription {
  explicit BufferDescription(std::string Name = {}, int Id = 0)
      : DisplayName(std::move(Name)), Id(Id) {}

  // A display name for the buffer. Not necessary unique.
  std::string DisplayName;
  // An optional buffer id used for target-specific analysis.
  int Id;
};

// This is the simulation log that contains state transitions and data for
// analyses.
class SimulationLog {
 public:
  explicit SimulationLog(
      const std::vector<BufferDescription>& BufferDescriptions);

  struct Line {
    unsigned Cycle;
    size_t BufferIndex;
    // A tag that identifies how to interpret `Msg`.
    std::string MsgTag;
    std::string Msg;
  };

  // Statistics about iterations.
  struct IterationStats {
    // Cycle when the last instruction completed.
    unsigned EndCycle;
  };

  // Returns the number of completed iterations.
  size_t GetNumCompleteIterations() const;

  std::string DebugString() const;

  // Buffer descriptions, one per buffer. Line::BufferIndex refers to these.
  const std::vector<BufferDescription> BufferDescriptions;
  // Log lines. Lines are guaranteed to be sorted by increasing cycle.
  std::vector<Line> Lines;

  std::vector<IterationStats> Iterations;
  unsigned NumCycles = 0;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_FRAMEWORK_LOG_H_
