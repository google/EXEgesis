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

#include "llvm_sim/framework/simulator.h"

#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {

namespace {

// A Logger that writes to a SimulationLog.
class LoggerImpl : public Logger {
 public:
  LoggerImpl(SimulationLog* Log, size_t BufferIndex, unsigned Cycle)
      : Log_(Log), BufferIndex_(BufferIndex), Cycle_(Cycle) {}

  void Log(std::string MsgTag, std::string Msg) override {
    Log_->Lines.push_back(
        {Cycle_, BufferIndex_, std::move(MsgTag), std::move(Msg)});
  }

 private:
  SimulationLog* const Log_;
  const size_t BufferIndex_;
  const unsigned Cycle_;
};

}  // namespace

class Simulator::IterationCounterSink : public Sink<InstructionIndex> {
 public:
  LLVM_NODISCARD bool PushMany(
      const std::vector<InstructionIndex::Type>& Elems) final {
    Elems_.insert(Elems_.end(), Elems.begin(), Elems.end());
    return true;
  }

  // Retrieves the last batch of elements, and resets them.
  std::vector<InstructionIndex::Type> RetrieveElems() {
    auto Tmp = std::move(Elems_);
    Elems_.clear();  // std::vector has no defined moved-from state.
    return Tmp;
  }

 private:
  std::vector<InstructionIndex::Type> Elems_;
};

Simulator::Simulator()
    : InstructionSink_(absl::make_unique<IterationCounterSink>()) {}

Simulator::~Simulator() {}

void Simulator::AddBuffer(std::unique_ptr<Buffer> Buf,
                          const BufferDescription& BufferDescription) {
  Buffers_.push_back(std::move(Buf));
  BufferDescriptions_.push_back(BufferDescription);
}

void Simulator::AddComponent(std::unique_ptr<Component> Comp) {
  Components_.push_back(std::move(Comp));
}

Sink<InstructionIndex>* Simulator::GetInstructionSink() const {
  return InstructionSink_.get();
}

std::unique_ptr<SimulationLog> Simulator::Run(const BlockContext& BlockContext,
                                              unsigned MaxNumIterations,
                                              unsigned MaxNumCycles) const {
  assert((MaxNumIterations > 0 || MaxNumCycles > 0) && "running forever ?");

  auto Result = absl::make_unique<SimulationLog>(BufferDescriptions_);

  // Set up components.
  for (const auto& Component : Components_) {
    Component->Init();
  }
  for (size_t BufferId = 0; BufferId < Buffers_.size(); ++BufferId) {
    LoggerImpl Logger(Result.get(), BufferId, 0);
    Buffers_[BufferId]->Init(&Logger);
  }

  // Run simulation.
  for (Result->NumCycles = 0;
       MaxNumCycles == 0 || Result->NumCycles < MaxNumCycles;
       ++Result->NumCycles) {
    for (const auto& Component : Components_) {
      Component->Tick(&BlockContext);
    }
    for (size_t BufferId = 0; BufferId < Buffers_.size(); ++BufferId) {
      LoggerImpl Logger(Result.get(), BufferId, Result->NumCycles);
      Buffers_[BufferId]->Propagate(&Logger);
    }

    // Update iteration stats.
    for (const auto& Instr : InstructionSink_->RetrieveElems()) {
      if (Instr.BBIndex == BlockContext.GetNumBasicBlockInstructions() - 1) {
        assert(Instr.Iteration == Result->Iterations.size() &&
               "simulation is not in order");
        SimulationLog::IterationStats Iteration;
        Iteration.EndCycle = Result->NumCycles;
        Result->Iterations.push_back(Iteration);
        // Stop simulation if the max number of iterations has been reached.
        if (MaxNumIterations > 0 && Instr.Iteration + 1 >= MaxNumIterations) {
          ++Result->NumCycles;
          return Result;
        }
      }
    }
  }

  return Result;
}

}  // namespace simulator
}  // namespace exegesis
