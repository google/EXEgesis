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

// Utilities to instantiate operands. Given an instruction with the list of its
// operand specifications (e.g. {"imm8", "r32", "r32"}), we want to generate a
// list of operand *instances* that we can use to generate code for this
// instruction. For the example above, and example would be
// {"0x42", "eax", "ecx"}.

#ifndef EXEGESIS_X86_OPERAND_TRANSLATOR_H_
#define EXEGESIS_X86_OPERAND_TRANSLATOR_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

// Instanciates all operands in the instructions.
InstructionFormat InstantiateOperands(const InstructionProto& instruction);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_OPERAND_TRANSLATOR_H_
