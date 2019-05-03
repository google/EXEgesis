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

// This is a simplified model of a bunch of execution units. This is typically
// used on architectures where execution units are hidden behind issue ports.
// In that case, execution units themselves are assumed to never be the
// bottleneck, but execution latency still needs to be modeled.
// The SimplifiedExecutionUnit component pulls any element from the source,
// keeps it `latency - 1` cycles, and then writes it back to the sink, so that
// they are available to be consumed after `latency` cycles. If the sink is not
// ready to accept elements, the execution unit will stall.
// See ":execution_unit" if you want to model execution units more precisely.

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_SIMPLIFIED_EXECUTION_UNITS_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_SIMPLIFIED_EXECUTION_UNITS_H_

#include <deque>
#include <string>

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// Note that `ElemTag` should have a `Latency` member.
template <typename ElemTag>
class SimplifiedExecutionUnits : public Component {
 public:
  struct Config {};

  SimplifiedExecutionUnits(const GlobalContext* Context, const Config& Config,
                           Source<ElemTag>* Source, Sink<ElemTag>* Sink)
      : Component(Context), Config_(Config), Source_(Source), Sink_(Sink) {}

  ~SimplifiedExecutionUnits() override {}

  void Init() { Elements_.clear(); }
  void Tick(const BlockContext* BlockContext) final;

 private:
  const Config Config_;
  Source<ElemTag>* const Source_;
  Sink<ElemTag>* const Sink_;
  std::vector<typename ElemTag::Type> Elements_;
};

// A buffer that always accepts elements. Elements spend `Latency-1` cycles in
// the buffer before being propagated.
// Note that `ElemTag` should have a `Latency` member.
template <typename ElemTag>
class ExecDepsBuffer : public Buffer,
                       public Source<ElemTag>,
                       public Sink<ElemTag> {
 public:
  ~ExecDepsBuffer() override {}

  void Init(Logger* Log) override {
    PendingElements_.clear();
    ReadyElements_.clear();
  }

  LLVM_NODISCARD bool PushMany(
      const std::vector<typename ElemTag::Type>& Elems) final {
    for (const auto& Elem : Elems) {
      PendingElements_.push_back(Elem);
    }
    return true;
  }

  const typename ElemTag::Type* Peek() const final {
    return ReadyElements_.empty() ? nullptr : &ReadyElements_.back();
  }

  void Pop() final {
    assert(!ReadyElements_.empty());
    ReadyElements_.pop_back();
  }

  void Propagate(Logger* Log) final;

 private:
  std::vector<typename ElemTag::Type> PendingElements_;
  std::vector<typename ElemTag::Type> ReadyElements_;
};

namespace internal {

// Decreases all non-zero latencies in `*Elems`.
template <typename T>
void DecreaseLatencies(std::vector<T>* Elems) {
  // Decrease latencies.
  for (auto& Elem : *Elems) {
    if (Elem.Latency > 0) {
      --Elem.Latency;
    }
  }
}

// Pops an element `E` with latency zero from `*Elems` until `Pred(E)` returns
// true. Zero-latency elements are processed in any order.
template <typename T, typename PredT>
void PopZeroLatencyElementsWhile(std::vector<T>* Elems, const PredT& Pred) {
  // Move all zero-latency elements to the end.
  std::partition(Elems->begin(), Elems->end(),
                 [](T& Elem) { return Elem.Latency > 0; });
  // Pop elements.
  while (!Elems->empty() && Elems->back().Latency == 0 && Pred(Elems->back())) {
    Elems->pop_back();
  }
}

}  // namespace internal

template <typename ElemTag>
void SimplifiedExecutionUnits<ElemTag>::Tick(const BlockContext* BlockContext) {
  // Read new elements.
  while (const auto* Elem = Source_->Peek()) {
    assert(Elem->Latency > 0);
    Elements_.push_back(*Elem);
    Source_->Pop();
  }

  internal::DecreaseLatencies(&Elements_);
  internal::PopZeroLatencyElementsWhile(
      &Elements_,
      [this](const typename ElemTag::Type& E) { return Sink_->Push(E); });
}

template <typename ElemTag>
void ExecDepsBuffer<ElemTag>::Propagate(Logger* Log) {
  internal::DecreaseLatencies(&PendingElements_);
  internal::PopZeroLatencyElementsWhile(
      &PendingElements_, [this, Log](const typename ElemTag::Type& E) {
        Log->Log(ElemTag::kTagName, ElemTag::Format(E));
        ReadyElements_.push_back(E);
        return true;
      });
}
}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_SIMPLIFIED_EXECUTION_UNITS_H_
