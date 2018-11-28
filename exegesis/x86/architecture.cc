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

#include "exegesis/x86/architecture.h"

#include <string>
#include <utility>

#include "exegesis/util/instruction_syntax.h"
#include "exegesis/x86/instruction_encoding.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

X86Architecture::X86Architecture(
    std::shared_ptr<const ArchitectureProto> architecture_proto)
    : Architecture(std::move(architecture_proto)) {
  BuildIndex();
}

void X86Architecture::BuildIndex() {
  for (InstructionIndex instruction_index(0);
       instruction_index < num_instructions(); ++instruction_index) {
    const InstructionProto& instruction_proto = instruction(instruction_index);
    CHECK(instruction_proto.has_x86_encoding_specification())
        << instruction_proto.DebugString();
    const uint32_t opcode_value =
        instruction_proto.x86_encoding_specification().opcode();
    const Opcode opcode(opcode_value);
    instruction_index_by_opcode_[opcode].push_back(instruction_index);

    if (!instruction_proto.x86_encoding_specification().has_vex_prefix()) {
      for (uint32_t prefix = opcode_value >> 8; prefix != 0;
           prefix = prefix >> 8) {
        legacy_opcode_prefixes_.insert(Opcode(prefix));
      }
    }
  }
}

OpcodeSet X86Architecture::GetOpcodes() const {
  OpcodeSet opcodes;
  gtl::InsertKeysFromMap(instruction_index_by_opcode_, &opcodes);
  return opcodes;
}

const std::vector<Architecture::InstructionIndex>*
X86Architecture::GetCandidates(const Opcode opcode) const {
  const std::vector<InstructionIndex>* candidates =
      gtl::FindOrNull(instruction_index_by_opcode_, opcode);
  if (candidates == nullptr) {
    // Sometimes three least significant bits of the instruction are used to
    // encode an operand. In that case the database will have this opcodes with
    // these bits zeroed out, so let's try to search for it.
    candidates = gtl::FindOrNull(instruction_index_by_opcode_,
                                 Opcode(opcode.value() & 0xFFFFFFF8));
    if (candidates == nullptr) {
      return nullptr;
    }

    // Check there are some instructions with operand encoded in opcode.
    for (const InstructionIndex& candidate : *candidates) {
      if (encoding_specification(candidate).operand_in_opcode() !=
          EncodingSpecification::NO_OPERAND_IN_OPCODE) {
        return candidates;
      }
    }
    return nullptr;
  }
  return candidates;
}

namespace {

bool InstructionMatchesSpecification(
    const EncodingSpecification& encoding_specification,
    const DecodedInstruction& decoded_instruction,
    const InstructionFormat& instruction_format, bool check_modrm) {
  return PrefixesAndOpcodeMatchSpecification(encoding_specification,
                                             decoded_instruction) &&
         (!check_modrm ||
          ModRmUsageMatchesSpecification(
              encoding_specification, decoded_instruction, instruction_format));
}

}  // namespace

X86Architecture::InstructionIndex X86Architecture::GetInstructionIndex(
    const DecodedInstruction& decoded_instruction, bool check_modrm) const {
  const Opcode opcode(decoded_instruction.opcode());
  const std::vector<InstructionIndex>* const candidates = GetCandidates(opcode);
  if (candidates != nullptr) {
    for (const InstructionIndex candidate_index : *candidates) {
      const EncodingSpecification& specification =
          encoding_specification(candidate_index);
      if (InstructionMatchesSpecification(
              specification, decoded_instruction,
              GetAnyVendorSyntaxOrDie(instruction(candidate_index)),
              check_modrm)) {
        return candidate_index;
      }
    }
  }
  return kInvalidInstruction;
}

std::vector<X86Architecture::InstructionIndex>
X86Architecture::GetInstructionIndices(
    const DecodedInstruction& decoded_instruction, bool check_modrm) const {
  std::vector<X86Architecture::InstructionIndex> indices;
  const Opcode opcode(decoded_instruction.opcode());
  const std::vector<InstructionIndex>* const candidates = GetCandidates(opcode);
  if (candidates != nullptr) {
    for (const InstructionIndex candidate_index : *candidates) {
      const EncodingSpecification& specification =
          encoding_specification(candidate_index);
      if (InstructionMatchesSpecification(
              specification, decoded_instruction,
              GetAnyVendorSyntaxOrDie(instruction(candidate_index)),
              check_modrm)) {
        indices.push_back(candidate_index);
      }
    }
  }
  return indices;
}

std::vector<X86Architecture::InstructionIndex>
X86Architecture::GetInstructionIndicesByOpcode(Opcode opcode) const {
  std::vector<InstructionIndex> indices;
  const std::vector<InstructionIndex>* const candidates = GetCandidates(opcode);
  if (candidates != nullptr) {
    indices = *candidates;
  }
  return indices;
}
}  // namespace x86
}  // namespace exegesis
