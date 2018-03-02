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

// An instruction decoder decodes instructions into individual uops.

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_DECODER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_DECODER_H_

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// See top comment.
class InstructionDecoder : public Component {
 public:
  struct Config {
    // TODO(courbet): Model the fact that not all decoders can handle all
    // instructions. For example, on Intel `Core`, the first decoder handles
    // instructions up to 4 uops in length, and the other 3 only handle single
    // uop instructions.
    // TODO(courbet): Implement the Loop Cache Buffer.
    int NumDecoders;  // Each decoder handles one instruction.
  };

  InstructionDecoder(const GlobalContext* Context, const Config& Config,
                     Source<InstructionIndex>* Source, Sink<UopId>* Sink);

  ~InstructionDecoder() override;

  void Tick(const BlockContext* BlockContext) override;

 private:
  const Config Config_;
  Source<InstructionIndex>* const Source_;
  Sink<UopId>* const Sink_;
};

}  // namespace simulator
}  // namespace exegesis
#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_DECODER_H_
