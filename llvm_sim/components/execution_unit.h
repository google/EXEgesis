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

// An execution unit is a component that executes uops. At each cycle, it can
// consume an element from an IssuePort and start executing it. At each cycle,
// the element will progress through the various stages of the execution unit.
// ExecutionUnits can be /pipelined/, in which case it can execute several
// elements simultaneously, though it can only start executing one per cycle.
// When an element is done executing (it reaches the end of the pipeline), it is
// written back to the sink. If the sink is not ready to accept elements, the
// execution unit will stall.

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_EXECUTION_UNIT_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_EXECUTION_UNIT_H_

#include <deque>
#include <string>

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// Note that ElemTag should have a Latency member, and the execution
// unit will only execute elements that match its own latency.
template <typename ElemTag>
class NonPipelinedExecutionUnit : public Component {
 public:
  struct Config {
    // The number of execution stages.
    unsigned NumStages;
  };

  NonPipelinedExecutionUnit(const GlobalContext* Context, const Config& Config,
                            Source<ElemTag>* Source, Sink<ElemTag>* Sink)
      : Component(Context), Config_(Config), Source_(Source), Sink_(Sink) {
    assert(Config_.NumStages > 0);
  }

  ~NonPipelinedExecutionUnit() override {}

  // Component API.
  void Init() final { CurStage_ = -1; }
  void Tick(const BlockContext* BlockContext) final {
    if (CurStage_ < 0) {
      // No executing element.
      StartNextElement();
      return;
    }

    const unsigned LastStage = Config_.NumStages - 1;
    if (CurStage_ < LastStage) {
      // Currently executing element is not done, go to next stage.
      ++CurStage_;
      return;
    }

    // Element is done.
    assert(CurStage_ == LastStage);
    if (Sink_->Push(Elem_)) {
      StartNextElement();
    }
  }

 private:
  // Try to start the next element from the source.
  void StartNextElement() {
    CurStage_ = -1;
    if (const auto* Elem = Source_->Peek()) {
      // TODO(courbet): This works for X86. Other targets might have different
      // ways to dispatch, and we might need to extract the criterion for
      // "being able to execute an element" into an ExecutionPolicy object.
      // Options are:
      //   - The ExecutionPolicy is a second template parameter to
      //     NonPipelinedExecutionUnit.
      //   - The ExecutionPolicy is an interface of which a pointer is passed in
      //     the config.
      if (Elem->Latency == Config_.NumStages) {
        CurStage_ = 0;
        Elem_ = *Elem;
        Source_->Pop();
      }
    }
  }

  const Config Config_;
  Source<ElemTag>* const Source_;
  Sink<ElemTag>* const Sink_;
  typename ElemTag::Type Elem_;
  // The current stage, or -1 for no element.
  int CurStage_;
};

template <typename ElemTag>
class PipelinedExecutionUnit : public Component {
 public:
  struct Config {
    // The number of execution stages.
    // TODO(courbet): Some execution units have a variable number of stages
    // depending on the data. Can we model this ?
    unsigned NumStages;
    // This is how many steps each execution stage takes.
    unsigned NumCyclesPerStage;
  };

  PipelinedExecutionUnit(const GlobalContext* Context, const Config& Config,
                         Source<ElemTag>* Source, Sink<ElemTag>* Sink)
      : Component(Context),
        Config_(Config),
        Source_(Source),
        Sink_(Sink),
        Pipeline_(Config_.NumStages) {
    assert(Config_.NumStages > 0);
  }

  ~PipelinedExecutionUnit() override {}

  // Component API.
  void Init() final;
  void Tick(const BlockContext* BlockContext) final;

 private:
  const Config Config_;
  Source<ElemTag>* const Source_;
  Sink<ElemTag>* const Sink_;
  struct PipelineElem {
    bool IsBubble = true;
    typename ElemTag::Type Elem;
  };
  std::deque<PipelineElem> Pipeline_;
  // The current cycle within the stage, in [0; NumCyclesPerStage[.
  int CurStageCycle_;
};

template <typename ElemTag>
void PipelinedExecutionUnit<ElemTag>::Init() {
  for (auto& Elem : Pipeline_) {
    Elem.IsBubble = true;
  }
  // Grab an element on next Tick().
  CurStageCycle_ = Config_.NumCyclesPerStage - 1;
}

template <typename ElemTag>
void PipelinedExecutionUnit<ElemTag>::Tick(const BlockContext* BlockContext) {
  // Each NumCyclesPerStage ticks, resources can progress in the pipeline.
  if (++CurStageCycle_ < Config_.NumCyclesPerStage) {
    return;
  }
  CurStageCycle_ = 0;

  if (!Pipeline_.back().IsBubble) {
    // The last element is done executing, try to push it to the sink (and stall
    // on failure).
    if (!Sink_->Push(Pipeline_.back().Elem)) {
      return;  // Stall.
    }
  }

  // Move the pipeline forward.
  PipelineElem Elem;
  if (const auto* SourceElem = Source_->Peek()) {
    if (SourceElem->Latency == Config_.NumStages * Config_.NumCyclesPerStage) {
      Elem.IsBubble = false;
      Elem.Elem = *SourceElem;
      Source_->Pop();
    }
  }
  Pipeline_.pop_back();
  Pipeline_.push_front(Elem);
}

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_EXECUTION_UNIT_H_
