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

#ifndef EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_PROPERTIES_H_
#define EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_PROPERTIES_H_

#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;

// Adds the missing feature flags for some cases where they are missing in the
// SDM.
Status AddMissingCpuFlags(InstructionSetProto* instruction_set);

// Adds the minimum required protection mode for instructions that require it.
// TODO(courbet): Ideally this would be parsed from the SDM, but the information
// is not stored in a consistent format (and sometimes not at given all).
Status AddProtectionModes(InstructionSetProto* instruction_set);

// Fixes the 'available in 64 bits' status of certain instructions that are
// marked as "unavailable except when they are available" in the SDM.
Status FixAvailableIn64Bits(InstructionSetProto* instruction_set);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_CLEANUP_INSTRUCTION_SET_PROPERTIES_H_
