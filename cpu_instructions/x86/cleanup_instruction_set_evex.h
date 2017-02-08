// Copyright 2017 Google Inc.
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

// Contains the library of InstructionSetProto transformations that add semantic
// information for EVEX-encoded instructions.

#ifndef THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_
#define THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_

#include "cpu_instructions/proto/instructions.pb.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {

using ::cpu_instructions::util::Status;

// Adds the EVEX.b bit interpretation field to all instructions in the
// instruction set.
Status AddEvexBInterpretation(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace cpu_instructions

#endif  // THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_
