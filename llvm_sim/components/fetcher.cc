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

#include "llvm_sim/components/fetcher.h"

#include <limits>

#include "llvm/MC/MCCodeEmitter.h"

namespace exegesis {
namespace simulator {

Fetcher::Fetcher(const GlobalContext* Context, const Config& Config,
                 Sink<InstructionIndex>* Sink)
    : Component(Context), Config_(Config), Sink_(Sink) {}

Fetcher::~Fetcher() {}

void Fetcher::Init() {
  InstructionIndex_ = {0, 0};
  InstrSizes_.clear();
}

void Fetcher::Tick(const BlockContext* BlockContext) {
  if (InstrSizes_.empty()) {
    ComputeInstructionSizes(BlockContext);
  }

  // Build a block of instructions such that the cumulative size is less than
  // MaxBytesPerCycle.
  int RemainingBytes = Config_.MaxBytesPerCycle;
  const auto NumBBInstrs = BlockContext->GetNumBasicBlockInstructions();
  if (InstructionIndex_.BBIndex >= NumBBInstrs) {
    if (BlockContext->IsLoop()) {
      // Start the next iteration.
      InstructionIndex_.BBIndex = 0;
      ++InstructionIndex_.Iteration;
    } else {
      return;  // We're done with the fetching.
    }
  }
  // The Fetcher has a fixed-size window over the code and cannot `see` the
  // looping instructions in the same cycle.
  while (RemainingBytes > 0 && InstructionIndex_.BBIndex < NumBBInstrs) {
    const unsigned InstrBytes = InstrSizes_[InstructionIndex_.BBIndex];
    if (InstrBytes > RemainingBytes) {
      return;
    }
    if (!Sink_->Push(InstructionIndex_)) {
      return;
    }
    RemainingBytes -= InstrBytes;
    ++InstructionIndex_.BBIndex;
  }
}

void Fetcher::ComputeInstructionSizes(const BlockContext* BlockContext) {
  InstrSizes_.resize(BlockContext->GetNumBasicBlockInstructions());
  llvm::SmallVector<llvm::MCFixup, 4> Fixups;
  for (size_t I = 0; I < BlockContext->GetNumBasicBlockInstructions(); ++I) {
    const llvm::MCInst& Inst = BlockContext->GetInstruction(I);
    unsigned& InstrBytes = InstrSizes_[I];
    InstrBytes = Context.InstrInfo->get(Inst.getOpcode()).getSize();
    if (InstrBytes > 0) {
      continue;
    }
    // If the instruction has variable size, we encode it to compute its size.
    std::string EncodedInstr;
    llvm::raw_string_ostream OS(EncodedInstr);
    Context.CodeEmitter->encodeInstruction(Inst, OS, Fixups,
                                           *Context.SubtargetInfo);
    InstrBytes = OS.str().size();
    assert(InstrBytes > 0);
  }
}

}  // namespace simulator
}  // namespace exegesis
