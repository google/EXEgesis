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

// A library of common functions used by the instruction set transforms.

#ifndef CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_
#define CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_

#include "cpu_instructions/proto/instructions.pb.h"

namespace cpu_instructions {
namespace x86 {

// Adds the operand size override prefix to the binary encoding specification of
// the given instruction proto. If the instruction already has the prefix, it is
// not added and a warning is printed to the log.
void AddOperandSizeOverrideToInstructionProto(InstructionProto* instruction);

}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_
