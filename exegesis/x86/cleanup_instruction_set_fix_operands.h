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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_FIX_OPERANDS_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_FIX_OPERANDS_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Updates the operands of CMPS and MOVS instructions. These instructions are
// documented in the Intel manual in two forms: a form that doesn't use any
// operands, and that encodes the size of its operands using a suffix of the
// mnemonic, and a form that uses explicit operands (even though all the
// registers in these operands are hard-coded and they can't be changed).
// The operand-less version is just fine, but the version with operands uses
// m8/m16/m32/m64 for the memory operand, even though this type is used also
// for memory operands specified through the ModR/M byte and allowing any
// addressing mode. This transform fixes this problem by changng the memory
// operands to more explicit ones.
Status FixOperandsOfCmpsAndMovs(InstructionSetProto* instruction_set);

// Updates the operands of INS and OUTS instructions. These instructions are
// documented in the Intel manual in two forms: a form that doesn't use any
// operands, and that encodes the size of its operands using a suffix of the
// mnemonic, and a form that uses explicit operands (even though all the
// registers in these operands are hard-coded and they can't be changed).
// The operand-less version is just fine, but the version with operands uses
// m8/m16/m32/m64 for the memory operand, even though this type is used also
// for memory operands specified through the ModR/M byte and allowing any
// addressing mode. This transform fixes this problem by changng the memory
// operands to more explicit ones.
Status FixOperandsOfInsAndOuts(InstructionSetProto* instruction_set);

// Updates the operands of the LDDQU instruction. In the SDM, the SSE version of
// the instruction uses "mem" for the memory operand, whereas it should be using
// "m128", similar to the 128-bit AVX version of the instruction.
Status FixOperandsOfLddqu(InstructionSetProto* instruction_set);

// Updates the operands of LODS, SCAS and STOS instructions. These instructions
// are documented in the Intel manual in two forms: a form that doesn't use any
// operands and that encodes the size of its operands using a suffix of the
// mnemonic, and a form that uses explicit operands (even though all the
// registers in these operands are hard-coded and they can't be changed).
// The operand-less version is just fine, but the version with operands has two
// validity/consistency problems:
// 1. It does not specify the register operand (even though it is required
//    according to the textual description of the instruction), and
// 2. it uses m8/m16/m32/m64 for the memory operand, even though this type is
//    otherwise used for memory operands specified through the ModR/M byte and
//    allowing any addressing mode.
// This transform fixes this problem by adding the register operand and by
// changng the memory operand to something more explicit.
Status FixOperandsOfLodsScasAndStos(InstructionSetProto* instruction_set);

// Updates the operands of SGTD and SIDT instructions. In the Intel manual, they
// are listed as SGTD m and SITD m, suggesting that they compute the effective
// address of the operand, but do not actually access the memory at this
// address. However, this is not the case, and they both write an 80-bit value
// at the operand. According to another part of the SDM, the correct operand
// type of these instructions is m16&32.
Status FixOperandsOfSgdtAndSidt(InstructionSetProto* instruction_set);

// Fixes the operands of VMOVQ. The Intel manual lists two variants of VMOVQ for
// XMM registers: one that reads the value from another XMM registers, and one
// that reads it from a location in memory. Both of these instructions use the
// same encoding, and the only difference is in how they use the ModR/M byte.
// This transform turns the second operand into xmm2/m64 for all occurences of
// this instruction; Removing the duplicate entries is left to
// RemoveDuplicateInstructions.
// Note that the transform must run before AddOperandInfo and
// RemoveDuplicateInstructions.
Status FixOperandsOfVMovq(InstructionSetProto* instruction_set);

// Fixes the ambiguous operand "reg". There are two cases in the 2015 version
// of the manual:
// 1. Instruction LAR. In this case, reg means any of r32 and r64, and the
//    binary encoding of these two versions is actually different (one uses the
//    REX.W prefix, the other doesn't, at least according to LLVM).
// 2. All other instructions. Here, the manual says that reg means r32 or r64,
//    but the instruction assigns a value to the lowest 16 bits of the register
//    and fills the rest with zeros. The binary encoding is the same for both
//    operand sizes. In practice, we can view this as an instruction that works
//    only on the 32-bit argument, and depends on the standard 64-bit register
//    behavior, i.e. whenever a 32-bit register is changed, the bits of its
//    64-bit extension are automatically filled with zeros.
//
// This transform fixes the reg operand in a way that depends on the mnemonic of
// the instruction. If the instruction is LAR, it replaces the 'reg' entry with
// two new entries, for r32 and r64.
// For all other instructions from the current version of the manual, it just
// renames reg to r32.
// TODO(ondrasej): Assemblers actually support the 64-bit version of all
// instructions fixed by this transform. Instead of making them 32-bit only, we
// might want to add a 64-bit entries to be compatible with what the assemblers
// do.
Status FixRegOperands(InstructionSetProto* instruction_set);

// Removes the implicit ST(0) operand from instructions that do not require it
// and where it is not produced by the LLVM disassembler. In all cases, this
// operand is encoded neither in the ModR/M byte nor in the opcode (using the
// "opcode+i" encoding). The instructions from which the operand is removed are
// selected by their binary encoding specification, because the mnemonic is not
// enough (two different instructions with the same mnemonic might work with the
// ST(0) register in different ways).
// Note that this transform depends on the results of RenameOperands.
Status RemoveImplicitST0Operand(InstructionSetProto* instruction_set);

// Removes the implicit xmm0 operand. The operand is added automatically by the
// LLVM assembler, but it is encoded neither in the ModR/M byte nor in the
// opcode of the instruction (using the "+i" encoding), and it does not appear
// in the LLVM disassembly. The Intel manual uses a special name <XMM0> for the
// implicit use of the operand, and this transform matches it only by its name.
Status RemoveImplicitXmm0Operand(InstructionSetProto* instruction_set);

// Inspects the operands of the instructions and renames them so that the names
// are consistent across types of operands. All of these renamings are either
// synonyms used by the Intel manual in different contexts, or the types are
// equivalent for 32- and 64-bit code.
Status RenameOperands(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_FIX_OPERANDS_H_
