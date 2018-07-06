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

// A library of helper functions for working with the instruction encoding
// protos defined in instruction_encoding.proto.
//
// For more details on the binary encoding of the instructions, see the wiki
// http://wiki.osdev.org/X86-64_Instruction_Encoding or the Intel 64 and IA-32
// Architectures Software Developer's Manual, Vol 2A, Section 2.1.3.

#ifndef EXEGESIS_X86_INSTRUCTION_ENCODING_H_
#define EXEGESIS_X86_INSTRUCTION_ENCODING_H_

#include <cstdint>
#include <string>
#include <vector>

#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "exegesis/util/bits.h"
#include "exegesis/util/index_type.h"
#include "exegesis/x86/instruction_encoding_constants.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

// Returns true if evex_b_interpretation is a value broadcast. Otherwise,
// returns false.
inline bool IsEvexBBroadcast(EvexBInterpretation evex_b_interpretation) {
  return evex_b_interpretation == EVEX_B_ENABLES_32_BIT_BROADCAST ||
         evex_b_interpretation == EVEX_B_ENABLES_64_BIT_BROADCAST;
}

// Returns mod field of a ModR/M byte. Result is shifted to the right so that
// the LSB of the field is the LSB of returned value.
inline uint8_t GetModRmModBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 6, 8);
}

// Returns reg field of a ModR/M byte. Result is shifted to the right so that
// the LSB of the field is the LSB of returned value.
inline uint8_t GetModRmRegBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 3, 6);
}

// Returns rm field of a ModR/M byte. Result is shifted to the right so that the
// LSB of the field is the LSB of returned value.
inline uint8_t GetModRmRmBits(uint8_t modrm_byte) {
  return GetBitRange(modrm_byte, 0, 3);
}

// Returns true if 'prefix_is_present' matches the requirements in
// 'specification'.
inline bool PrefixMatchesSpecification(
    LegacyEncoding::PrefixUsage specification, bool prefix_is_present) {
  switch (specification) {
    case LegacyEncoding::PREFIX_IS_REQUIRED:
      return prefix_is_present;
    case LegacyEncoding::PREFIX_IS_NOT_PERMITTED:
      return !prefix_is_present;
    case LegacyEncoding::PREFIX_IS_IGNORED:
      return true;
    case LegacyEncoding::PREFIX_USAGE_IS_UNKNOWN:
      LOG(DFATAL) << "Prefix state is unknown for an instruction";
      return true;
    default:
      LOG(FATAL) << "Invalid prefix specification value: " << specification;
      return false;
  }
}

// -----------------------------------------------------------------------------
//  Functions for validation and inspection of instructions
// -----------------------------------------------------------------------------

// Returns the number of bytes needed to encode a displacement value from ModR/M
// and SIB. Returns 0 if there is no displacement value.
int NumModRmDisplacementBytes(const ModRm& modrm, const Sib& sib);

// Returns true if the combination of values in 'modrm' require an additional
// SIB byte. This is true when indirect addressing mode is used and the value of
// the modrm.rm operand is 4.
bool ModRmRequiresSib(const ModRm& modrm);

// Validates the mandatory prefix bits in the VEX or EVEX prefix. Returns an
// error when the mandatory prefix in the specification differs from the
// mandatory prefix in the encoding. Otherwise, returns Status::OK.
template <typename PrefixType>
Status ValidateMandatoryPrefixBits(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    const PrefixType& prefix) {
  if (vex_prefix_specification.mandatory_prefix() !=
      prefix.mandatory_prefix()) {
    return InvalidArgumentError(
        "The mandatory prefix in the specification does not match the "
        "mandatory prefix in the instruction.");
  }
  return OkStatus();
}

// Validates the map select bits in the VEX or EVEX prefix. Returns an error
// when the map select bits in the prefix differ from the map select in the
// specification, or if they are VexPrefix::UNDEFINED_MAP_SELECT. Otherwise,
// returns Status::OK.
template <typename PrefixType>
Status ValidateMapSelectBits(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    const PrefixType& prefix) {
  if (vex_prefix_specification.map_select() != prefix.map_select()) {
    return InvalidArgumentError(
        "The opcode map selector in the specification does not match the "
        "opcode map selector in the instruction.");
  }
  if (prefix.map_select() == VexEncoding::UNDEFINED_OPERAND_MAP) {
    return InvalidArgumentError(
        "UNDEFINED_OPERAND_MAP must not be used in the encoding.");
  }
  return OkStatus();
}

// Validates the vector length bits of a VEX or EVEX prefix. Returns Status::OK
// if the bits conform to the specification, and InvalidArgumentError otherwise.
// Returns a FailedPreconditionError if the vector length from the specification
// is not supported by the selected prefix type or if it is not valid.
Status ValidateVectorSizeBits(VexVectorSize vector_size_specification,
                              uint32_t vector_length_or_rounding_bits,
                              VexPrefixType prefix_type);

// Validates the register operand encoded in the VEX or EVEX prefix. Returns an
// error when vex_operand_usage does not allow adding an operand and the operand
// bits is different from all zeros and all ones. Otherwise, returns Status::OK.
Status ValidateVexRegisterOperandBits(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    uint32_t vex_register_operand);

// Validates the (e)vex.w bit of a VEX or EVEX prefix. Returns Status::OK if the
// bit conforms to the specification, and InvalidArgumentError otherwise.
// Returns a FailedPreconditionError if the (e)vex.w usage specification is not
// valid.
Status ValidateVexWBit(VexPrefixEncodingSpecification::VexWUsage vex_w_usage,
                       bool vex_w_bit);

// Validates the EVEX.b bit of an EVEX prefix. Returns Status::OK if the bit
// conforms to the specification, and InvalidArgumentError otherwise. Returns
// a FailedPreconditionError if the specification is not for an EVEX prefix.
Status ValidateEvexBBit(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    const DecodedInstruction& decoded_instruction);

// Returns the EVEX.b interpretation that is used in case of
// 'decoded_instruction'. Assumes (but does not check) that
// 'vex_prefix_specification' is an encoding specification of the EVEX prefix
// for the same instruction. Returns UNDEFINED_EVEX_B_INTERPRETATION when the
// instruction does not use the EVEX.b bit.
EvexBInterpretation GetUsedEvexBInterpretation(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    const DecodedInstruction& decoded_instruction);

// Validates the EVEX.aaa bits of an EVEX prefix. Returns Status::OK if the bits
// conform to the specification, and InvalidArgumentError otherwise. Returns a
// FailedPreconditionError if the specification is not for an EVEX prefix.
Status ValidateEvexOpmask(
    const VexPrefixEncodingSpecification& vex_prefix_specification,
    const DecodedInstruction& decoded_instruction);

// -----------------------------------------------------------------------------
//  Functions for generating instances of instructions
// -----------------------------------------------------------------------------

// Creates a base decoded instruction proto for the given specification. Sets
// the value of all fields that are uniquely determined by the specification.
// Explicitly adds the prefix and ModR/M submessages even if they use only the
// default values.
DecodedInstruction BaseDecodedInstruction(
    const EncodingSpecification& specification);

// Generates possible combinations of instruction encodings for a given
// instruction. The generated encodings will include at least one example of
// each major addressing mode. It will also generate combinations of prefixes
// allowed by the function.
// For example, for functions that do not use the REX prefix by default, the
// function will generate versions of the instruction that force its presence.
// Similarly, for instructions that support the two-byte VEX prefix, it also
// generates versions that use the three-byte form of the prefix. The function
// may skip some combinations, if a similar encoding is already generated.
// The function CHECK_fails if the instruction specification is not valid, e.g.
// if the binary encoding specification is missing.
std::vector<DecodedInstruction> GenerateEncodingExamples(
    const InstructionProto& instruction);

// Checks that the contents of the ModR/M byte and SIB byte, if used, match the
// encoding specification and the operand addressing modes of the instruction.
bool ModRmUsageMatchesSpecification(
    const EncodingSpecification& specification,
    const DecodedInstruction& instruction,
    const InstructionFormat& instruction_format);

// Returns true if 'instruction' matches 'specification' based on its prefixes
// and opcode. The function does not make any attempt to match the operands of
// the instruction, because its main purpose is to find the right specification
// while decoding an instruction, and the operands (e.g. the ModR/M byte and the
// immediate values are not parsed yet).
bool PrefixesAndOpcodeMatchSpecification(
    const EncodingSpecification& specification,
    const DecodedInstruction& instruction);

// Checks whether the contents of the ModR/M and SIB bytes (if present) match
// the given addressing mode from InstructionOperand::AddressingMode.
bool ModRmAddressingModeMatchesInstructionOperandAddressingMode(
    const DecodedInstruction& decoded_instruction,
    InstructionOperand::AddressingMode rm_operand_addressing_mode);

// -----------------------------------------------------------------------------
//  Functions for manually assigning operands of instructions
// -----------------------------------------------------------------------------

// The index of an x86-64 register. The numerical values of the register indices
// correspond to the numerical value used to encode them in the x86-64 binary
// encoding.
// Note that some registers may use the same index. The x86-64 architecture uses
// several disjoint set of registers: general purpose registers, floating point
// stack registers, XMM registers and SSE/AVX registers. Moreover, many of these
// register exist in multiple sizes (8 to 64 bits for general purpose
// registers), and for example the registers AL, AX, RAX, XMM0 all have the same
// index. Which register set and what data size is used by a particular
// instruction is determined by the opcode and the prefixes of the instruction.
DEFINE_INDEX_TYPE(RegisterIndex, int);

// A register index that is never used as a "real" register index. This value is
// returned by GetRegisterIndex if the register name is not recognized.
extern const RegisterIndex kInvalidRegisterIndex;

// Translates a symbolic name of an x86-64 register to a register index, i.e.
// the value used in the binary encoding of the instructions to represent the
// register. Returns kInvalidRegisterIndex if 'register_name' is not a name of a
// known register.
RegisterIndex GetRegisterIndex(std::string register_name);

// Assigns a register to the 'operand_position'-th operand of the instruction
// specified by 'instruction_format'; the register is assigned to the encoded
// version of the instruction in 'instruction'. The function does not make any
// attempt to validate that 'instruction' belongs to the same instruction as
// 'instruction_format'; moreover, it assumes that 'instruction_format', has the
// InstructionFormat.operands field filled correctly, and that 'instruction' has
// its prefix data structures set up properly.
// The function returns an error if the operand cannot be assigned, or if the
// operand or register index are not valid.
Status SetOperandToRegister(const InstructionFormat& instruction_format,
                            int operand_position, RegisterIndex register_index,
                            DecodedInstruction* instruction);

// Functions for assigning a memory location to operands. There is one function
// for each addressing mode supported by the x86-64 architecture. Note that the
// number of variants of the function may seem more complex than necessary, but
// this is on purpose - even though some groups of addressing modes are the same
// from the point of view of a user (and the Intel assembly syntax), they
// actually use two different encodings with different instruction sizes.

// Assigns the operand encoded in modrm.rm to a memory location addressed
// indirectly by an absolute address provided encoded in the instruction. This
// encoding uses both the ModR/M and the SIB bytes.
Status SetOperandToMemoryAbsolute(const InstructionFormat& instruction_format,
                                  uint32_t absolute_address,
                                  DecodedInstruction* instruction);

// Assigns the operand encoded in modrm.rm to a memory location addressed
// indirectly by the absolute address in 'base_register'; the operand is encoded
// only through the ModR/M byte, without the use of the SIB byte. Note that
// register indices 4, 5, 12, and 13 are used as escape values for the SIB byte
// and for RIP-relative addressing, and they can't be used as base registers
// with this function. However, they can be encoded through the SIB byte.
// Assembly example: MOV CX, [RBX]
Status SetOperandToMemoryBase(const InstructionFormat& instruction_format,
                              RegisterIndex base_register,
                              DecodedInstruction* instruction);

// Assigns the operand encoded in modrm.rm to a memory location addressed
// indirectly by the absolute address in 'base_register'; the operand is encoded
// through the SIB byte. Note that register indices 5 and 13 are not allowed,
// because they serve as escape values for indirect addressing by an absolute
// address; on the other hand, this encoding is the only way how to encode
// indirect addressing by RSP and R12.
// Assembly example: MOV CX, [RSP]
Status SetOperandToMemoryBaseSib(const InstructionFormat& instruction_format,
                                 RegisterIndex base_register,
                                 DecodedInstruction* instruction);

// Assigns the operand encoded in modrm.rm to a memory location addressed
// indirectly by RIP and a 32-bit displacement; the operand is encoded only
// through the ModR/M byte (addressing mode is INDIRECT, and modrm.rm operand is
// set to 5), without the use of the SIB byte.
// Assembly example: MOV CX, [RIP - 64]
Status SetOperandToMemoryRelativeToRip(
    const InstructionFormat& instruction_format, int32_t displacement,
    DecodedInstruction* instruction);

// Assigns the operand ecnoded in modrm.rm to a memory location addressed
// indirectly by the absolute address in 'base_register' and an 8/32-bit
// displacement; the operand is encoded through the ModR/M byte and a one-byte
// displacement value. Note that register indices 4 and 12 are used as escape
// values for the SIB byte, and they can't be used as base registers with this
// function. However, they can be encoded through the SIB byte.
// Assembly example: MOV CX, [RBX + 12]
Status SetOperandToMemoryBaseAnd8BitDisplacement(
    const InstructionFormat& instruction_format, RegisterIndex base_register,
    int8_t displacement, DecodedInstruction* instruction);
Status SetOperandToMemoryBaseAnd32BitDisplacement(
    const InstructionFormat& instruction_format, RegisterIndex base_register,
    int32_t displacement, DecodedInstruction* instruction);

// Extracts precise addressing mode from decoded instruction proto by taking
// ModR/M and SIB usage into account.
// NOTE(ondrasej): Note that this function might not guess the addressing mode
// correctly, e.g. for LEA instructions. It should not be used for matching a
// DecodedInstruction with InstructionOperands.
InstructionOperand::AddressingMode ConvertToInstructionOperandAddressingMode(
    const DecodedInstruction& decoded_instruction);

// TODO(ondrasej): Implement the functions for the remaining addressing modes.

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_INSTRUCTION_ENCODING_H_
