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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MERGE_INSTRUCTIONS_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MERGE_INSTRUCTIONS_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Merges instructions that are synonyms, i.e. they have the same encoding and
// the same addressing modes of their operands. Replaces the synonymical
// instructions with a single instructions that has all the synonymical vendor
// syntaxes. Such synonyms are used in the Intel assembly syntax for the
// convenience of the developers, and the instructions differ either in the
// order of the operands (XCHG) or in the presence of implicitly-encoded
// arguments (e.g. STOSB).
//
// Returns an error when the instructions have the same encoding specification
// and addressing modes, but they differ in other details.
//
// Examples of instructions updated by this transform:
//   XCHG m32, r32 / XCHG r32, m32
//   STOS BYTE PTR [RDI], STOSB
Status MergeVendorSyntax(InstructionSetProto* instruction_set);

// Finds instructions that:
// - have more than one vendor syntax,
// - all its syntaxes are equivalent in the sense that they have the same
//   mnemonic and all operands have (pointwise) the same names and the same
//   addressing modes.
// Removes all vendor syntaxes of such instructions except for the first one.
Status RemoveUselessOperandPermutations(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_MERGE_INSTRUCTIONS_H_
