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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds the EVEX.b bit interpretation field to all instructions in the
// instruction set.
Status AddEvexBInterpretation(InstructionSetProto* instruction_set);

// Adds the opmask-related fields to the encoding specifications of all
// instructions in the instruction set.
Status AddEvexOpmaskUsage(InstructionSetProto* instruction_set);

// Moves the AVX-512 embedded-rounding and suppress-all-exceptions tags {er} and
// {sae} to a separate operand, to match the syntax used by actual instruction.
// This is necessary, because the SDM lists these as properties of the last
// operand of the instruction, but the assembly syntax for these instructions
// introduces by the manual puts a comma between them and the last operand.
Status AddEvexPseudoOperands(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_EVEX_H_
