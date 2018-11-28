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

#ifndef EXEGESIS_X86_ARCHITECTURE_H_
#define EXEGESIS_X86_ARCHITECTURE_H_

#include <memory>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "exegesis/base/architecture.h"
#include "exegesis/base/opcode.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace x86 {

// A specialization of the ::exegesis::Architecture class for the x86-64
// architecture; it provides functions that are specific to the x86-64
// encoding:
// * Looking up instructions by their opcodes and prefixes,
// * Looking up instructions matching a given decoded instruction.
// * Examining the ModR/M usage of instructions.
class X86Architecture : public Architecture {
 public:
  // Creates the architecture object.
  explicit X86Architecture(
      std::shared_ptr<const ArchitectureProto> architecture_proto);

  // Disallow copy and assign.
  X86Architecture(const X86Architecture&) = delete;
  X86Architecture& operator=(const X86Architecture&) = delete;

  // Returns the list of all possible opcodes. The opcodes include the mandatory
  // prefixes (0F, 0F 38, 0F 3A). For VEX-encoded instructions, these prefixes
  // are obtained from the opcode map bits and properly translated into the
  // legacy prefix values.
  OpcodeSet GetOpcodes() const;

  // Returns the list of indices of instructions with the given opcode. Returns
  // an empty list if the opcode does not exist.
  std::vector<InstructionIndex> GetInstructionIndicesByOpcode(
      Opcode opcode) const;

  // Returns the index of the instruction that best matches 'instruction'.
  // Returns kInvalidInstruction if no matching instruction is found.
  // If check_modrm is set, it also tries to match modrm fields of the
  // instruction with the encoding specifications in the database.
  InstructionIndex GetInstructionIndex(
      const DecodedInstruction& decoded_instruction, bool check_modrm) const;

  // Returns all of the indices that match the given instruction with prefixes.
  // Returns an empty vector if no matching instruction is found.
  // If check_modrm is set, it also tries to match modrm fields of the
  // instruction with the encoding specifications in the database.
  std::vector<X86Architecture::InstructionIndex> GetInstructionIndices(
      const DecodedInstruction& decoded_instruction, bool check_modrm) const;

  // Returns the instruction encoding specification for the index-th
  // instruction.
  const EncodingSpecification& encoding_specification(
      InstructionIndex index) const {
    DCHECK_GE(index, 0);
    DCHECK_LT(index, num_instructions());
    return instruction(index).x86_encoding_specification();
  }

  // Returns the instruction proto that matches with the decoded instruction.
  const InstructionProto& GetInstructionOrDie(
      const DecodedInstruction& decoded_instruction) const {
    const InstructionIndex instruction_index =
        GetInstructionIndex(decoded_instruction, true);
    CHECK_NE(instruction_index, kInvalidInstruction);
    return instruction(instruction_index);
  }

  // Returns true if 'prefix' is a proper prefix of an opcode of a legacy
  // instruction. Note that we're using big endian, and the prefixes are
  // "packed" towards the lower bytes, so the prefixes of 0x0F01D5 are 0x0F01
  // and 0x0F.
  bool IsLegacyOpcodePrefix(Opcode prefix) const {
    return legacy_opcode_prefixes_.contains(prefix);
  }

 private:
  // Builds the index of instructions. This method CHECK-fails if the data in
  // the architecture proto are invalid.
  void BuildIndex();

  // Returns possible candidates for the given opcode, also handling the case
  // where three least significant bits of the instruction are used to encode an
  // operand. In such case it looks for the opcode with these bits set to zero.
  // If no match is found, it returns nullptr.
  const std::vector<InstructionIndex>* GetCandidates(Opcode opcode) const;

  // Instructions in instruction_set_ indexed by their opcode. Note that there
  // typically are multiple instructions with the same opcode (e.g. a version
  // with REX prefix and a version without it).
  // TODO(ondrasej): We should use a version of vector that does not
  // heap-allocate small vectors.
  absl::flat_hash_map<Opcode, std::vector<InstructionIndex>, Opcode::Hasher>
      instruction_index_by_opcode_;

  // The set of proper prefixes of opcodes of legacy instructions in the
  // database.
  absl::flat_hash_set<Opcode, Opcode::Hasher> legacy_opcode_prefixes_;
};

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_ARCHITECTURE_H_
