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

#include "exegesis/x86/cleanup_instruction_set_rex.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/base/opcode.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "exegesis/x86/cleanup_instruction_set_utils.h"
#include "glog/logging.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

LegacyEncoding::PrefixUsage GetRexWUsage(const InstructionProto& instruction) {
  DCHECK(instruction.has_x86_encoding_specification());
  const EncodingSpecification& encoding_specification =
      instruction.x86_encoding_specification();
  DCHECK(encoding_specification.has_legacy_prefixes());
  return encoding_specification.legacy_prefixes().rex_w_prefix();
}

void SetRexWUsage(InstructionProto* instruction,
                  LegacyEncoding::PrefixUsage usage) {
  DCHECK(instruction != nullptr);
  DCHECK(instruction->has_x86_encoding_specification());
  EncodingSpecification* const encoding_specification =
      instruction->mutable_x86_encoding_specification();
  encoding_specification->mutable_legacy_prefixes()->set_rex_w_prefix(usage);
}

}  // namespace

Status AddRexWPrefixUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  AddPrefixUsageToLegacyInstructions(GetRexWUsage, SetRexWUsage,
                                     instruction_set);
  return OkStatus();
}
// NOTE(ondrasej): This cleanup must run right after parsing the encoding
// specification.
REGISTER_INSTRUCTION_SET_TRANSFORM(AddRexWPrefixUsage, 1020);

}  // namespace x86
}  // namespace exegesis
