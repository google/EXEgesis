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

// Contains consistency checks for the instruction database. The functions
// defined in this library do not modify the instruction set; they are launched
// at the end of the pipeline, and they return an error status if the
// instruction set is not consistent.

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_CHECKS_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_CHECKS_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Checks that the opcodes of all instructions have the correct format, i.e. one
// of the following applies:
// * The opcode has only one byte.
// * The opcode has two bytes, and the first byte is 0F.
// * The opcode has three bytes, and the first two bytes are either 0F 38 or
//   0F 3A.
Status CheckOpcodeFormat(InstructionSetProto* instruction_set);

// Checks that all operands of all instructions have the all their properties
// filled.
Status CheckOperandInfo(InstructionSetProto* instruction_set);

// Checks that no instruction with a multi-byte opcode is a special case of
// another instruction with a shorter opcode and a ModR/M byte specification.
Status CheckSpecialCaseInstructions(InstructionSetProto* instruction_set);

// Checks that all instructions have at least one vendor syntax.
Status CheckHasVendorSyntax(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_CHECKS_H_
