// Copyright 2016 Google Inc.
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

// Contains the library of InstructionSetProto transformations used for cleaning
// up the instruction database obtained from the Intel manuals.

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_ALTERNATIVES_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_ALTERNATIVES_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Replaces every instruction with a register/memory operand with one
// corresponding instruction that has the register operand, and another one
// with the memory operand. For example XOR r16,r/m16 will be replaced by the
// two instructions XOR r16,r16 and XOR r16,m16.
Status AddAlternatives(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_ALTERNATIVES_H_
