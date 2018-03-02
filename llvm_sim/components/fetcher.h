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

// An instruction fetcher fetches a block instructions from memory. The block of
// instructions should have a total encoded size smaller than MaxBytesPerCycle.

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_FETCHER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_FETCHER_H_

#include <deque>
#include <vector>

#include "llvm_sim/components/buffer.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// See top comment.
class Fetcher : public Component {
 public:
  struct Config {
    int MaxBytesPerCycle;
  };

  Fetcher(const GlobalContext* Context, const Config& Config,
          Sink<InstructionIndex>* Sink);

  ~Fetcher() override;

  void Init() override;
  void Tick(const BlockContext* BlockContext) override;

 private:
  void ComputeInstructionSizes(const BlockContext* BlockContext);

  const Config Config_;
  Sink<InstructionIndex>* const Sink_;
  // The index of the next instruction to fetch.
  InstructionIndex::Type InstructionIndex_;
  // A cache of instruction sizes.
  std::vector<unsigned> InstrSizes_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_FETCHER_H_
