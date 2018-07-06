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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_

#include <functional>

#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"

namespace exegesis {
namespace x86 {

// Adds the operand size override prefix to the binary encoding specification of
// the given instruction proto. If the instruction already has the prefix, it is
// not added and a warning is printed to the log.
void AddOperandSizeOverrideToInstructionProto(InstructionProto* instruction);

// Functions that get/set prefix usage of some prefix of an instruction. To be
// used together with AddPrefixToLegacyInstructions.
using GetPrefixUsage = LegacyEncoding::PrefixUsage(const InstructionProto&);
using SetPrefixUsage = void(InstructionProto*, LegacyEncoding::PrefixUsage);

// Adds prefix usage to all legacy instructions, using the following rules:
// 1. The instructions are grouped by their opcode + optional opcode extension.
// 2. When the group has an instruction where the prefix usage is not
//    PREFIX_USAGE_IS_UNKNOWN, it sets PREFIX_IS_NOT_PERMITTED to all
//    instructions in the group where the current usage is
//    PREFIX_USAGE_IS_UNKNOWN.
// 3. Otherwise, it sets PREFIX_USAGE_IS_IGNORED to all instructions in the
//    group.
// The functions get_prefix and set_prefix are used to get/set the value of
// prefix usage for a single instruction.
void AddPrefixUsageToLegacyInstructions(
    const std::function<GetPrefixUsage>& get_prefix,
    const std::function<SetPrefixUsage>& set_prefix,
    InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_UTILS_H_
