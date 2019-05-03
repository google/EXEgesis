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

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_SET_UTILS_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_SET_UTILS_H_

#include <string>

#include "absl/container/flat_hash_set.h"

namespace exegesis {
namespace x86 {

// Returns the set of x87 FPU instruction mnemonics.
const absl::flat_hash_set<std::string>& GetX87FpuInstructionMnemonics();

}  // namespace x86
}  // namespace exegesis

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_SET_UTILS_H_
