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

// Port pressure analysis. Computes the average number of cycles that each port
// was busy dispatching per iteration.

#ifndef EXEGESIS_LLVM_SIM_ANALYSIS_PORT_PRESSURE_H_
#define EXEGESIS_LLVM_SIM_ANALYSIS_PORT_PRESSURE_H_

#include <vector>

#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {

// Result of PortPressure analysis. Port pressure is computed for all buffers
// that log "PortPressure" metadata.
class PortPressureAnalysis {
 public:
  struct PortPressure {
    // Index of the buffer in the simulation log.
    size_t BufferIndex = 0;
    // How many cycles per loop iteration was this port busy for.
    float CyclesPerIteration = 0.0;
    // How many cycles per iteration was the given MCInst dispatched on this
    // port.
    // Note that `sum(CyclesByMCInst) = CyclesPerIteration`.
    std::vector<float> CyclesPerIterationByMCInst;
  };

  std::vector<PortPressure> Pressures;
};

// Computes the port pressure.
PortPressureAnalysis ComputePortPressure(const BlockContext& BlockContext,
                                         const SimulationLog& Log);

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_ANALYSIS_PORT_PRESSURE_H_
