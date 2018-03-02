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

#ifndef EXEGESIS_LLVM_SIM_FRAMEWORK_SIMULATOR_H_
#define EXEGESIS_LLVM_SIM_FRAMEWORK_SIMULATOR_H_

#include <memory>
#include <vector>

#include "llvm/MC/MCInst.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm_sim/framework/component.h"
#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {

class Simulator {
 public:
  Simulator();
  ~Simulator();

  // Adds a buffer/component.
  void AddBuffer(std::unique_ptr<Buffer> Buf,
                 const BufferDescription& BufferDescription);
  void AddComponent(std::unique_ptr<Component> Comp);

  // Returns a sink that receives instructions that are done executing. This is
  // used by the simulator to count iterations. Typically used as last step of a
  // simulation pipeline. Owned by the simulator. The Sink's PushMany()
  // implementation always returns true.
  Sink<InstructionIndex>* GetInstructionSink() const;

  // Runs the simulator until either the max number of iterations or cycles has
  // been reached (`0` means no limit)
  // TODO(courbet): Should the simulator work on a MachineBasicBlock rather than
  // a vector<MCInst> ? Right now we don't really know where we're going to get
  // our data from, so we use vector<MCInst> (which is the common denominator).
  std::unique_ptr<SimulationLog> Run(const BlockContext& BlockContext,
                                     unsigned MaxNumIterations,
                                     unsigned MaxNumCycles) const;

 private:
  class IterationCounterSink;
  const std::unique_ptr<IterationCounterSink> InstructionSink_;
  std::vector<std::unique_ptr<Buffer>> Buffers_;
  std::vector<BufferDescription> BufferDescriptions_;
  std::vector<std::unique_ptr<Component>> Components_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_FRAMEWORK_SIMULATOR_H_
