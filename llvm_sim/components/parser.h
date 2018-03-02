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

// An instruction parser with a limited bandwidth.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_PARSER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_PARSER_H_

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

class InstructionParser : public Component {
 public:
  struct Config {
    int MaxInstructionsPerCycle;
  };

  InstructionParser(const GlobalContext* Context, const Config& Config,
                    Source<InstructionIndex>* Source,
                    Sink<InstructionIndex>* Sink);

  ~InstructionParser() override;

  void Tick(const BlockContext* BlockContext) override;

 private:
  const Config Config_;
  Source<InstructionIndex>* const Source_;
  Sink<InstructionIndex>* const Sink_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_PARSER_H_
