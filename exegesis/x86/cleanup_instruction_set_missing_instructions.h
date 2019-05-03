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

// Contains cleanups that add missing (undocumented) x86-64 instructions.

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MISSING_INSTRUCTIONS_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MISSING_INSTRUCTIONS_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds the undocumented instruction FFREEP. For more information about the
// instruction see https://www.pagetable.com/?p=16.
Status AddMissingFfreepInstruction(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MISSING_INSTRUCTIONS_H_
