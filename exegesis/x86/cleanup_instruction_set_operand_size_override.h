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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_SIZE_OVERRIDE_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_SIZE_OVERRIDE_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds the missing operand size override prefix to the binary encoding
// specification of instruction where it is missing. We detect such instructions
// finding groups of instructions that have the same binary encoding, but where
// some of them use 16-bit operand, while others use 32-bit operands.
// Note that this transform depends on operand types being added to the vendor
// syntax section of the instruction.
Status AddOperandSizeOverridePrefix(InstructionSetProto* instruction_set);

// Adds operand size override prefix usage to the encoding specifications of the
// legacy instructions in 'instruction_set'. This transform must run after other
// transforms modifying the operand size override status.
Status AddOperandSizeOverridePrefixUsage(InstructionSetProto* instruction_set);

// Adds the operand size override prefix to 16-bit versions of instructions with
// implicit operands. Because these instructions have no operand, we have no way
// of detecting the 16-bit version other than through their mnemonics.
Status AddOperandSizeOverrideToInstructionsWithImplicitOperands(
    InstructionSetProto* instruction_set);

// Adds the operand size override prefix to 16-bit versions of instructions
// where the generic 16-bit detection fails. This function handles instructions
// where there are two versions with two different sizes, but the sizes are not
// strictly 16-bit and 32-bit. They are typically either 16/64-bit instructions
// or 32/48-bit instructions (16-bit selector + 16/32-bit offset).
Status AddOperandSizeOverrideToSpecialCaseInstructions(
    InstructionSetProto* instruction_set);

// Adds another version with operand size override for instructions that
// existence of an operand size override is optional.
Status AddOperandSizeOverrideVersionForSpecialCaseInstructions(
    InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_SIZE_OVERRIDE_H_
