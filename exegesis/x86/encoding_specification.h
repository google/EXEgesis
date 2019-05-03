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

// Contains a parser and generator for the instruction encoding specification
// language used in the Intel manuals and a class that contains the information
// from the specification in a better accessible form.
//
// An informal specification of the language can be found in Intel 64 and IA-32
// Architectures Software Developer's Manual, Volume 2, Chapter 3.1.
//
// Note that the goal is not to support all the features of the specification
// language but rather to validate the instruction database. Fields that do not
// contribute to this goal are in general checked, but they are not exported by
// the parser.

#ifndef EXEGESIS_X86_ENCODING_SPECIFICATION_H_
#define EXEGESIS_X86_ENCODING_SPECIFICATION_H_

#include <string>
#include <unordered_set>

#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::StatusOr;

// Parses the instruction encoding specification from a string.
//
// Typical usage:
// StatusOr<EncodingSpecification> specification_or_status =
//     ParseEncodingSpecification("F3 0F AE /3");
// CHECK_OK(specification_or_status.status());
// printf("%x\n", specification_or_status.ValueOrDie().opcode());
StatusOr<EncodingSpecification> ParseEncodingSpecification(
    const std::string& specification);

// Generates an encoding specification string of the format given in the Intel
// Architecture manual.
std::string GenerateEncodingSpec(const InstructionFormat& instruction,
                                 const EncodingSpecification& encoding_spec);

// A collection of instruction operand encodings.
using InstructionOperandEncodingMultiset =
    std::unordered_multiset<InstructionOperand::Encoding, std::hash<int>>;

// Returns a set of operand encodings that can be used by an instruction. The
// set is determined from the binary encoding specification of the instruction.
// Note that for most of the operands, if they appear in the returned set, there
// *must* be some operand using this encoding. Note that the function does not
// return anything for implicit operands, because they are not encoded in the
// binary encoding specification.
InstructionOperandEncodingMultiset GetAvailableEncodings(
    const EncodingSpecification& specification);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_ENCODING_SPECIFICATION_H_
