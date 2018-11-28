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

// A parser for the binary encoding of x86-64 instructions. The parser takes a
// stream of bytes and extracts the instructions as DecodedInstruction protos.
//
// Typical usage:
//  X86Architecture architecture;
//  InstructionParser parser(&architecture);
//
//  StatusOr<DecodedInstruction> instruction_or_status =
//      parser.ParseBinaryEncoding({0x90});
//  if (instruction_or_status.ok()) { ... }
//
// or:
//  absl::Span<const uint8_t> binary_code = ...;
//  while (!binary_code.empty()) {
//    StatusOr<DecodedInstruction> instruction_or_status =
//        parser.ConsumeBinaryEncoding(&binary_code);
//    RETURN_IF_ERROR(instruction_or_status.status());
//
//    ...
//  }
//
// See http://wiki.osdev.org/X86-64_Instruction_Encoding for an overview of the
// x86-64 instruction encoding.

#ifndef EXEGESIS_X86_INSTRUCTION_PARSER_H_
#define EXEGESIS_X86_INSTRUCTION_PARSER_H_

#include <cstdint>

#include "absl/types/span.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "exegesis/x86/architecture.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// An implementation of a parser for the binary encoding of x86-64 instructions.
// Note that this class does not perform full disassembly, it only parses the
// instruction data to a structured form that is easy to manage and inspect.
//
// Note that the parser is not thread safe and one parser may not be used from
// multiple threads at the same time.
class InstructionParser {
 public:
  // Initializes the instruction parser with the given instruction set
  // information. The instruction_db object must remain valid for the whole
  // lifetime of the instruction parser.
  explicit InstructionParser(const X86Architecture* architecture);

  // Parses a single instruction from 'encoded_instruction'. This method updates
  // 'encoded_instruction' so that when an instruction is parsed correctly, it
  // will begin with the first byte of the following instruction. When the
  // method returns failure, the state of 'encoded_instruction' is valid but
  // undefined.
  StatusOr<DecodedInstruction> ConsumeBinaryEncoding(
      absl::Span<const uint8_t>* encoded_instruction);

  // Parses a single instruction from 'encoded_instruction'. Ignores all bytes
  // following the first instruction.
  StatusOr<DecodedInstruction> ParseBinaryEncoding(
      absl::Span<const uint8_t> encoded_instruction) {
    return ConsumeBinaryEncoding(&encoded_instruction);
  }

 private:
  // Resets the state of the parser.
  void Reset();

  // Parses the prefixes of the instruction. Expects that 'encoded_instruction'
  // starts with the first byte of the instruction. It removes the prefixes from
  // the span as they are parsed. On success, the span is updated so that it
  // starts with the first non-prefix byte of the instruction. When the method
  // fails, the state of the span is undefined.
  Status ConsumePrefixes(absl::Span<const uint8_t>* encoded_instruction);

  // Parses the REX prefix of the instruction.
  Status ParseRexPrefix(uint8_t prefix_byte);

  // When the first byte of 'encoded_instruction' is a segment override prefix,
  // parses the prefix, removes the byte from the span, and returns true.
  // Otherwise, does nothing and returns false.
  bool ConsumeSegmentOverridePrefixIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);
  // When the first byte of 'encoded_instruction' is an address size override
  // prefix, parses the prefix, removes the byte from the span, and returns
  // true. Otherwise, does nothing and returns false.
  bool ConsumeAddressSizeOverridePrefixIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);

  // Parses the VEX (resp. EVEX) prefix of the instruction. Expects that
  // 'encoded_instruction' starts with the first byte of the (E)VEX prefix. It
  // removes the (E)VEX prefix from the span as it is parsed. On success, the
  // span is updated so that it starts with the first non-prefix byte of the
  // instruction. When the method fails, the state of the span is undefined.
  Status ConsumeVexPrefix(absl::Span<const uint8_t>* encoded_instruction);
  Status ConsumeEvexPrefix(absl::Span<const uint8_t>* encoded_instruction);

  // Parses the opcode of the instruction. Expects that 'encoded_instruction'
  // starts with the first byte of the opcode and it removes the opcode from the
  // span as it is parsed. On success, the span is updated so that it starts
  // with the first byte of the instruction following the opcode. When the
  // method fails, the state of the span is undefined.
  Status ConsumeOpcode(absl::Span<const uint8_t>* encoded_instruction);

  // Gets the encoding specification for the given opcode, also handling the
  // case where three least significant bits of the instruction are used to
  // encode an operand. In such case it looks for the opcode with these bits set
  // to zero. check_modrm decides whether to match modrm byte of the
  // specification with the decoded instruction we have.
  const EncodingSpecification* GetEncodingSpecification(uint32_t opcode_value,
                                                        bool check_modrm);

  // Parses the contents of the ModR/M and SIB bytes if they are used by the
  // instruction. Assumes that the opcode of the instruction was already parsed
  // and that 'encoded_instruction' starts with the first byte after the opcode.
  // On success, the span is updated so that it starts with the first byte of
  // the instruction following the ModR/M and SIB bytes and any potential
  // displacement values. When the method fails, the state of the span is
  // undefined.
  Status ConsumeModRmAndSIBIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);

  // Parses the immediate value attached to the instruction if there is one.
  // Assumes that all parts of the instruction preceding the immediate value are
  // already parsed and that 'encoded_instruction' starts with the first byte
  // after the immediate value. On success, the span is updated so that it
  // starts with the first byte following the immediate value parsed by this
  // method. When the method fails, the state of the span is undefined.
  Status ConsumeImmediateValuesIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);

  // Parses the code offset attached to the instruction if there is one. Assumes
  // that all parts of the instruction preceding the code offset are already
  // parsed and that 'encoded_instruction' starts with the first byte after the
  // immediate value. On success, the span is updated so that it starts with
  // the first byte following the code offset parsed by this method. When the
  // method fails, the state of the span is undefined.
  Status ConsumeCodeOffsetIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);

  // Parses the value of the VEX suffix attached to the instruction if there is
  // one. Assumes that all parts of the instruction preceding the code offset
  // are already parsed and that 'encoded_instruction' starts with the first
  // byte after the immediate value. On success, the span is updated so that it
  // starts with the first byte following the VEX suffix parsed by this method.
  // When the method fails, the state of the span is undefined.
  Status ConsumeVexSuffixIfNeeded(
      absl::Span<const uint8_t>* encoded_instruction);

  // Methods for updating the prefixes from the four legacy prefix groups (see
  // http://wiki.osdev.org/X86-64_Instruction_Encoding#Legacy_Prefixes for the
  // description of the groups).
  //
  // All of these methods can be called at most once during the parsing of the
  // instruction. Multiple calls mean that the encoded instruction had more than
  // one prefix from the prefix group corresponding to the method. This is not
  // an error per se, but it leads to undefined behavior of the CPU and we want
  // to reject instructions like that.
  Status AddLockOrRepPrefix(LegacyEncoding::LockOrRepPrefix prefix);
  void AddSegmentOverridePrefix(LegacyEncoding::SegmentOverridePrefix prefix);
  Status AddOperandSizeOverridePrefix();
  void AddAddressSizeOverridePrefix();

  // The architecture information used by the parser. The architecture is used
  // to determine what parts of an instruction are present for a given
  // combination of opcode and prefixes.
  const X86Architecture* const architecture_;

  // The encoding specification of the current instruction. The specification is
  // retrieved when the parser finishes parsing the prefixes and the opcode of
  // the instruction - before that, the pointer is set to nullptr.
  // The encoding specification object is owned by 'instruction_db_'; it remains
  // valid as long as 'instruction_db_' is valid and it must not be deleted
  // explicitly.
  const EncodingSpecification* specification_;

  // The encoded form of the current instruction processed by the parser.
  absl::Span<const uint8_t> encoded_instruction_;

  // The current instruction processed by the parser.
  DecodedInstruction instruction_;
};

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_INSTRUCTION_PARSER_H_
