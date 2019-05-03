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

// Contains the library of InstructionSetProto transformations that add
// structured information about the operands of the instructions.

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_INFO_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_INFO_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds more implicit info about VMX instructions.
Status AddVmxOperandInfo(InstructionSetProto* instruction_set);

// VMFUNC uses EAX as input register but this info is not parseable by current
// heuristics. This transform manually defines it.
Status FixVmFuncOperandInfo(InstructionSetProto* instruction_set);

// Directly sets properties of the first operand of the MOVDIR64B instruction.
// The first (modrm.reg-encoded) operand is a general purpose register, but it
// is interpreted as an address.
Status AddMovdir64BOperandInfo(InstructionSetProto* instruction_set);

// Directly sets properties of the first operand of the UMONITOR instruction.
// The instruction accepts a single register
Status AddUmonitorOperandInfo(InstructionSetProto* instruction_set);

// Adds detailed information about operands to the vendor syntax section.
// Assumes that this section already has operand names in the format used by the
// Intel manual, and so is the encoding scheme of the instruction proto. This
// function replaces any existing operand information in the vendor syntax so
// that the i-th operand structure corresponds to the i-th operand of the
// instruction in the vendor syntax specification.
// Note that this instruction depends on the output of RenameOperands.
Status AddOperandInfo(InstructionSetProto* instruction_set);

// Applies heuristics to determine the usage patterns of operands with unknown
// usage patterns. For example, VEX.vvvv are implicitly read from except when
// specified. This transforms explictly sets usage to USAGE_READ. Also, implicit
// registers (e.g. in ADD AL, imm8) are usually missing usage in the SDM.
Status AddMissingOperandUsage(InstructionSetProto* instruction_set);

// Adds USAGE_READ to the last operadn of VBLEND instructions. As of May 2018,
// the operand usage is missing for the last operand across multiple versions of
// the instruction.
Status AddMissingOperandUsageToVblendInstructions(
    InstructionSetProto* instruction_set);

// Adds RegisterClass to every operand in vendor_syntax.
Status AddRegisterClassToOperands(InstructionSetProto* instruction_set);

// Adds VEX operand usage informamtion to instructions where it is missing. This
// information used to be a part of the instruction encoding specification in
// the SDM, but it was removed starting with the November 2018 version of the
// manual. This transform reconstructs the info from the other available
// information about the instruction.
Status AddMissingVexVOperandUsage(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_OPERAND_INFO_H_
