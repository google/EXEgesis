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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_REX_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_REX_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds missing REX.W prefix usage to the instructions: it groups legacy
// instructions by their opcodes/opcode extensions, and then in each group:
// - if the group contains a REX.W instruction, it sets 'REX.W is not permitted'
//   to other instructions in the group.
// - if the group does not contain a REX.W instruction, it sets 'REX.W is
//   ignored' to all instructions in the group.
// - it does not modify instructions that already have other REX.W usage set.
Status AddRexWPrefixUsage(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_REX_H_
