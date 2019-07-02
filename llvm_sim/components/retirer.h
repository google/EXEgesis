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

// A component that retires elements. It has two output sinks: One to mark
// retired elements and the other one to mark retired instructions (instructions
// for which all uops have been retired).
// It is assumed that uops for an instruction are pushed to the source in order.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_RETIRER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_RETIRER_H_

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

template <typename Tag>
struct UopFieldGetter {
  UopId::Type operator()(const typename Tag::Type& Elem) const {
    return Elem.Uop;
  }
};

// See top comment.
// UopIdGetter is used to retrieve the uop id from the Tag.
template <typename Tag, typename UopIdGetter = UopFieldGetter<Tag>>
class Retirer : public Component {
 public:
  struct Config {};

  Retirer(const GlobalContext* Context, const Config& Config,
          Source<Tag>* Source, Sink<Tag>* ElemSink,
          Sink<InstructionIndex>* RetiredInstructionsSink)
      : Component(Context),
        Config_(Config),
        GetOpId_(),
        Source_(Source),
        Sink_(ElemSink),
        RetiredInstructionsSink_(RetiredInstructionsSink) {}

  ~Retirer() override {}

  void Tick(const BlockContext* BlockContext) override {
    while (const auto* Elem = Source_->Peek()) {
      if (!Sink_->Push(*Elem)) {
        return;
      }
      const auto Uop = GetOpId_(*Elem);
      const auto& Decomposition = Context.GetInstructionDecomposition(
          BlockContext->GetInstruction(Uop.InstrIndex.BBIndex));
      if (Uop.UopIndex + 1 == Decomposition.Uops.size()) {
        // This is the last uop in the instruction.
        const bool Pushed = RetiredInstructionsSink_->Push(Uop.InstrIndex);
        assert(Pushed);
        (void)Pushed;
      }
      Source_->Pop();
    }
  }

 private:
  const Config Config_;
  const UopIdGetter GetOpId_;
  Source<Tag>* const Source_;
  Sink<Tag>* const Sink_;
  Sink<InstructionIndex>* const RetiredInstructionsSink_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_RETIRER_H_
