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

#ifndef EXEGESIS_X86_INSTRUCTION_ENCODER_H_
#define EXEGESIS_X86_INSTRUCTION_ENCODER_H_

#include <cstdint>
#include <vector>

#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::StatusOr;

// Encodes an x86-64 instruction according to the provided encoding
// specification and the values of the bits in 'decoded_instruction'. Returns
// the encoded instruction as a vector of bytes. Returns an error if
// 'specification' and 'instruction_data' are inconsistent, e.g. the number or
// sizes of the immediate values do not match or 'instruction_data' contains
// data for the ModR/M byte even though the instruction does not use it
// according to the binary specification.
StatusOr<std::vector<uint8_t>> EncodeInstruction(
    const EncodingSpecification& specification,
    const DecodedInstruction& decoded_instruction);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_INSTRUCTION_ENCODER_H_
