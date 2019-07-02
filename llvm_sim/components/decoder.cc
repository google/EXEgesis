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

#include "llvm_sim/components/decoder.h"

#include "llvm/MC/MCInstrItineraries.h"

namespace exegesis {
namespace simulator {

InstructionDecoder::InstructionDecoder(const GlobalContext* Context,
                                       const Config& Config,
                                       Source<InstructionIndex>* Source,
                                       Sink<UopId>* Sink)
    : Component(Context), Config_(Config), Source_(Source), Sink_(Sink) {}

InstructionDecoder::~InstructionDecoder() {}

void InstructionDecoder::Tick(const BlockContext* BlockContext) {
  int RemainingInstructions = Config_.NumDecoders;

  while (RemainingInstructions > 0 && Source_->Peek()) {
    const auto InstrIndex = *Source_->Peek();
    const auto& Decomposition = Context.GetInstructionDecomposition(
        BlockContext->GetInstruction(InstrIndex.BBIndex));
    const auto NumUops = Decomposition.Uops.size();
    std::vector<UopId::Type> UopIds(NumUops);
    for (size_t I = 0; I < NumUops; ++I) {
      UopIds[I] = {InstrIndex, I};
    }
    if (!Sink_->PushMany(UopIds)) {
      return;
    }
    Source_->Pop();
    --RemainingInstructions;
  }
}

}  // namespace simulator
}  // namespace exegesis
