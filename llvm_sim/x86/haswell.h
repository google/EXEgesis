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

#ifndef EXEGESIS_LLVM_SIM_TEST_HASWELL_H_
#define EXEGESIS_LLVM_SIM_TEST_HASWELL_H_

#include <memory>

#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/simulator.h"

namespace exegesis {
namespace simulator {

std::unique_ptr<Simulator> CreateHaswellSimulator(const GlobalContext& Context);

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_TEST_HASWELL_H_
