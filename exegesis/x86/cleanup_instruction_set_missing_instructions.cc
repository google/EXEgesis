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

#include "exegesis/x86/cleanup_instruction_set_missing_instructions.h"

#include "absl/strings/string_view.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

void AddInstructionGroup(absl::string_view instruction_group_proto,
                         absl::string_view instruction_proto,
                         InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const int new_group_index = instruction_set->instruction_groups_size();
  *instruction_set->add_instruction_groups() =
      ParseProtoFromStringOrDie<InstructionGroupProto>(instruction_group_proto);
  InstructionProto* const new_instruction = instruction_set->add_instructions();
  *new_instruction =
      ParseProtoFromStringOrDie<InstructionProto>(instruction_proto);
  new_instruction->set_instruction_group_index(new_group_index);
}

}  // namespace

Status AddMissingFfreepInstruction(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kFfreepMnemonic[] = "FFREEP";
  constexpr char kFfreepInstructionProto[] = R"proto(
    description: "Free Floating-Point Register and Pop."
    vendor_syntax {
      mnemonic: "FFREEP"
      operands {
        addressing_mode: DIRECT_ADDRESSING
        name: "ST(i)"
        usage: USAGE_WRITE
      }
    }
    available_in_64_bit: true
    legacy_instruction: true
    encoding_scheme: "M"
    raw_encoding_specification: "DF /0")proto";
  constexpr char kFfreepInstructionGroupProto[] = R"proto(
    name: "FFREEP"
    description: "Free Floating-Point Register and Pop."
    flags_affected { content: "" }
    short_description: "Free Floating-Point Register and Pop.")proto";
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    if (HasMnemonicInVendorSyntax(instruction, kFfreepMnemonic)) {
      return OkStatus();
    }
  }
  AddInstructionGroup(kFfreepInstructionGroupProto, kFfreepInstructionProto,
                      instruction_set);
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingFfreepInstruction, 0);

}  // namespace x86
}  // namespace exegesis
