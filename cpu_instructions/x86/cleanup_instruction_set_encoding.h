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

// Contains instruction set transforms that fix the binary encoding
// specifications of the instructions.

#ifndef CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_ENCODING_H_
#define CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_ENCODING_H_

#include "cpu_instructions/proto/instructions.pb.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {

using ::cpu_instructions::util::Status;

// Fixes the binary encoding specification of instructions that write to or read
// from a memory address that is specified as segment + fixed offset, and the
// offset is encoded as an immediate value in the instruction. For some of these
// instructions, namely MOV to/from a fixed offset do not have this immediate
// value in the binary encoding specification in the Intel manual. This might be
// because the size of the immediate value depends on the use of the address
// size override prefix. This transform fixes these instructions by replacing
// the original one with two new instructions (one with the prefix and one
// without) with the correct binary encoding specification.
Status AddMissingMemoryOffsetEncoding(InstructionSetProto* instruction_set);

// Adds the missing ModR/M and immediates specifiers to the binary encoding
// specification of instructions where they are missing. Most of these cases are
// actual errors in the Intel manual rather than conversion errors that could be
// fixed elsewhere.
Status AddMissingModRmAndImmediateSpecification(
    InstructionSetProto* instruction_set);

// Fixes and cleans up binary encodings of SET* instructions. These are
// instructions that look at a combination of status flag and update an 8-bit
// register or memory location based on the value of these flags.
// There are two problems with these instructions in the Intel manual:
// 1. All of them are missing the /r (or /0) specifier stating that there must
//    be a ModR/M byte.
// 2. The REX versions of the instructions are redundant, because the REX prefix
//    is used only for the register index extension bits.
// This transform adds the /0 specification (because the modrm.reg bits are not
// used for anything), and it removes the REX versions of the instructions.
Status FixAndCleanUpEncodingSpecificationsOfSetInstructions(
    InstructionSetProto* instruction_set);

// Fixes the binary encodings of POP FS and POP GS instructions. These
// instructions exist in three versions: 16-bit, 32-bit and 64-bit. In protected
// mode, either the 32-bit or the 64-bit is the default, depending on the
// default address size of the given segment.
// * In the 64-bit protected mode, the 64-bit version is the default, the 32-bit
//   version can't be encoded, and the 16-bit version can be obtained by using
//   the operand size override prefix. Adding a REX.W prefix to the instruction
//   does not change anything apart from the binary encoding size.
// * In the 32-bit protected mode, the 32-bit version is the default, the 64-bit
//   version can be obtained by using the REX.W prefix, and the 16-bit version
//   can be obtained by using the operand size override prefix.
// The Intel manual has all three versions, and they all appear without any
// prefixes at all. This transform adds the operand size override prefix to the
// 16-bit version, keeps the 32-bit version as is (it will be later removed as
// non-encodable anyway), keeps the 64-bit version as is (this will be kept as
// the default, since we're focusing on the 64-bit protected mode), and adds a
// new version of the 64-bit version that uses the REX.W prefix.
Status FixEncodingSpecificationOfPopFsAndGs(
    InstructionSetProto* instruction_set);

// Fixes the binary encodings of PUSH FS and PUSH GS instructions. These
// instructions exist in three versions symmetrical to the POP FS and POP GS
// instructions (see the comment on FixEncodingSpecificationOfPopFsAndGs
// for more details).
// The Intel manual lists only one version of each. This transform adds the
// missing versions and extends them with the necessary operand size override
// and REX.W prefixes.
Status FixEncodingSpecificationOfPushFsAndGs(
    InstructionSetProto* instruction_set);

// Fixes the binary encoding specification of the instruction XBEGIN. The
// specifications in the Intel manual have only the opcode, but there is also a
// code offset passed as an immediate value, and the 16-bit version of the
// instruction requires an operand-size override prefix.
Status FixEncodingSpecificationOfXBegin(InstructionSetProto* instruction_set);

// Fixes common errors in the binary encoding specification that were carried
// from the Intel reference manuals. Errors fixed by this transform are:
// 1. Replaces 0f with 0F,
// 2. Replaces imm8 with ib,
// 3. Replaces .0 at the end of a VEX prefix with .W0.
Status FixEncodingSpecifications(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_ENCODING_H_
