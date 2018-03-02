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

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_COMMON_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_COMMON_H_

#include "llvm/Support/Compiler.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// Component helpers.
struct RenamedUopId {
  struct Type {
    UopId::Type Uop;
    // Microarchitectural Registers that this uop uses/defines.
    llvm::SmallVector<size_t, 8> Uses;
    llvm::SmallVector<size_t, 8> Defs;
  };

  static const char kTagName[];
  static std::string Format(const Type& Elem);
  static const InstructionIndex::Type& GetInstructionIndex(const Type& Elem) {
    return Elem.Uop.InstrIndex;
  }
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_COMMON_H_
