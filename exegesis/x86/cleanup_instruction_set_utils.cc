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

#include "exegesis/x86/cleanup_instruction_set_utils.h"

#include <cstdint>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "glog/logging.h"

namespace exegesis {
namespace x86 {
namespace {

// The operand size override prefix used by the 16-bit versions of some
// instructions.
const char kOperandSizeOverridePrefix[] = "66 ";

}  // namespace

// Adds the operand size override prefix to the binary encoding specification of
// the given instruction proto. If the instruction already has the prefix, it is
// not added and a warning is printed to the log.
void AddOperandSizeOverrideToInstructionProto(InstructionProto* instruction) {
  CHECK(instruction != nullptr);
  if (instruction->has_x86_encoding_specification()) {
    // If the x86 encoding specification was not parsed yet, ignore it; it would
    // be overwritten later anyway.
    EncodingSpecification* const encoding_specification =
        instruction->mutable_x86_encoding_specification();
    LegacyPrefixEncodingSpecification* const legacy_prefixes =
        encoding_specification->mutable_legacy_prefixes();
    legacy_prefixes->set_operand_size_override_prefix(
        LegacyEncoding::PREFIX_IS_REQUIRED);
  }
  const std::string& raw_encoding_specification =
      instruction->raw_encoding_specification();
  if (!absl::StrContains(raw_encoding_specification,
                         kOperandSizeOverridePrefix)) {
    instruction->set_raw_encoding_specification(
        absl::StrCat(kOperandSizeOverridePrefix, raw_encoding_specification));
  } else {
    LOG(WARNING)
        << "The instruction already has an operand size override prefix: "
        << instruction->raw_encoding_specification();
  }
}

void AddPrefixUsageToLegacyInstructions(
    const std::function<GetPrefixUsage>& get_prefix,
    const std::function<SetPrefixUsage>& set_prefix,
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);

  absl::flat_hash_map<uint64_t, std::vector<InstructionProto*>>
      instructions_by_opcode;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    EncodingSpecification* const encoding_specification =
        instruction.mutable_x86_encoding_specification();
    if (encoding_specification->has_vex_prefix()) continue;
    // Some instructions have an opcode extension in ModR/M. Each opcode
    // extension defines a separate instruction, and we need to treat them as
    // such. To avoid conflicts, we use both the opcode and the opcode extension
    // in the key of instructions_by_opcode.
    const uint64_t opcode =
        static_cast<uint64_t>(encoding_specification->opcode()) << 32 |
        (encoding_specification->modrm_usage() ==
                 EncodingSpecification::OPCODE_EXTENSION_IN_MODRM
             ? encoding_specification->modrm_opcode_extension()
             : 0);
    instructions_by_opcode[opcode].push_back(&instruction);
  }

  for (const auto& instruction_elem : instructions_by_opcode) {
    const std::vector<InstructionProto*>& instructions =
        instruction_elem.second;
    const bool has_version_with_prefix_usage =
        std::any_of(instructions.begin(), instructions.end(),
                    [&](const InstructionProto* instruction) {
                      return get_prefix(*instruction) !=
                             LegacyEncoding::PREFIX_USAGE_IS_UNKNOWN;
                    });
    // When there is at least one version of the instruction with the prefix, we
    // disallow the prefix on all the other versions. Otherwise, we mark the
    // prefix as ignored.
    // TODO(ondrasej): Verify that the prefix is ignored in all the other cases.
    const LegacyEncoding::PrefixUsage prefix_usage =
        has_version_with_prefix_usage ? LegacyEncoding::PREFIX_IS_NOT_PERMITTED
                                      : LegacyEncoding::PREFIX_IS_IGNORED;
    for (InstructionProto* const instruction : instructions) {
      if (get_prefix(*instruction) == LegacyEncoding::PREFIX_USAGE_IS_UNKNOWN) {
        set_prefix(instruction, prefix_usage);
      }
    }
  }
}

}  // namespace x86
}  // namespace exegesis
