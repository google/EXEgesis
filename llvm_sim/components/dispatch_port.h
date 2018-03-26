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

// A dispatch port is simply a LinkBuffer with capacity NumUnits, plus some
// additional logging to allow analysis.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_DISPATCH_PORT_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_DISPATCH_PORT_H_

#include "llvm_sim/components/buffer.h"

namespace exegesis {
namespace simulator {

// See top comment.
// DispatchPort<ElemTag> requires ElemTag to define a:
// `static T GetInstructionIndex() const` method that returns the underlying
// instruction index. `T` can be `InstructionIndex::Type` or
// `const InstructionIndex::Type&`.
template <typename ElemTag>
class DispatchPort : public LinkBuffer<ElemTag> {
 public:
  explicit DispatchPort(size_t Capacity) : LinkBuffer<ElemTag>(Capacity) {}

  ~DispatchPort() override {}

  void Init(Logger* Log) override {
    LinkBuffer<ElemTag>::Init(Log);
    // Tell the port pressure analysis that we generate pressure information.
    Log->Log("PortPressure", "init");
  }

  void PrePropagate(Logger* Log,
                    const typename LinkBuffer<ElemTag>::QueueT& Pending) final {
    for (const auto& Elem : Pending) {
      const auto& InstrId = ElemTag::GetInstructionIndex(Elem);
      Log->Log("PortPressure", llvm::Twine(InstrId.Iteration)
                                   .concat(",")
                                   .concat(llvm::Twine(InstrId.BBIndex))
                                   .concat(",1")
                                   .str());
    }
  }
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_DISPATCH_PORT_H_
