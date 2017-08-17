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

#include "exegesis/x86/cleanup_instruction_set_checks.h"

#include <cstdint>
#include <unordered_set>

#include "base/stringprintf.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/status_util.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

void LogErrorAndUpdateStatus(const string& error_message,
                             const InstructionProto& instruction,
                             Status* status) {
  const Status error = InvalidArgumentError(
      StrCat(error_message, "\nInstruction:\n", instruction.DebugString()));
  // In case of the check cleanups, we want to print as many errors as possible
  // to make their analysis easier.
  LOG(WARNING) << error;
  UpdateStatus(status, error);
}

}  // namespace

Status CheckOpcodeFormat(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static constexpr uint32_t kOpcodeUpperBytesMask = 0xffffff00;
  const std::unordered_set<uint32_t> kAllowedUpperBytes = {0, 0x0f00, 0x0f3800,
                                                           0x0f3a00};
  // Also check that there are no opcodes that would be a prefix of a longer
  // multi-byte opcode.
  const std::unordered_set<uint32_t> kForbiddenOpcodes = {0x0f, 0x0f38, 0x0f3a};
  Status status = OkStatus();
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    if (!instruction.has_x86_encoding_specification()) {
      LogErrorAndUpdateStatus(
          "The instruction does not have an encoding specification.",
          instruction, &status);
      continue;
    }
    const EncodingSpecification& encoding_specification =
        instruction.x86_encoding_specification();
    const uint32_t opcode = encoding_specification.opcode();
    const uint32_t opcode_upper_bytes = opcode & kOpcodeUpperBytesMask;
    if (!ContainsKey(kAllowedUpperBytes, opcode_upper_bytes)) {
      LogErrorAndUpdateStatus(
          StringPrintf("Invalid opcode upper bytes: %x", opcode_upper_bytes),
          instruction, &status);
      continue;
    }
    if (ContainsKey(kForbiddenOpcodes, opcode)) {
      LogErrorAndUpdateStatus(StringPrintf("Invalid opcode: %x", opcode),
                              instruction, &status);
      continue;
    }
  }
  return status;
}
// TODO(ondrasej): Add this transform to the default pipeline when all problems
// it finds are resolved.

}  // namespace x86
}  // namespace exegesis
