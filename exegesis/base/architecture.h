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

// Declares the 'Architecture' class, a wrapper over exegesis::ArchitectureProto
// that provides efficient lookup for the data in the proto.

#ifndef EXEGESIS_BASE_ARCHITECTURE_H_
#define EXEGESIS_BASE_ARCHITECTURE_H_

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/index_type.h"
#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {

// Returns a unique_ptr to a Printer which prints all fields in index order and
// which correctly prints instruction_group_index fields even if they have value
// of 0.
std::unique_ptr<google::protobuf::TextFormat::Printer>
GetArchitectureProtoTextPrinter();

// Base class for classes that provide an indexing an lookup API for data stored
// in an ArchitectureProto. This class contains functionality that is common for
// all architectures. Certain architectures may inherit from this class and add
// functionality that is specific for that particular architecture.
//
//  std::shared_ptr<ArchitectureProto> instruction_set =
//      GetArchitectureProtoOrDie("registered:intel");
//  Architecture architecture(architecture);
//  for (const Architecture::InstructionIndex i :
//       architecture.GetInstructionsByVendorSyntax("MOV")) {
//    const InstructionProto& instruction = architecture.instruction(i);
//    ...
//  }
class Architecture {
 public:
  // An index of an instruction in the ISA. The indices of the instructions in
  // this class are an implementation detail that is internal to the given
  // Architecture object. The indices are safe to use with the Architecture
  // object that returned them, but they must not be used with other
  // Architecture objects.
  DEFINE_INDEX_TYPE(InstructionIndex, int);

  // An index of a microarchitecture in the ISA. The indices of the
  // microarchitectures in this class are an implementation detail that is
  // internal to the given Architecture object. The indices are safe to use with
  // the Architecture object that returned them, but they must not be used with
  // other Architecture objects.
  DEFINE_INDEX_TYPE(MicroArchitectureIndex, int);

  // An instruction index that is not used by any existing instruction.
  static constexpr InstructionIndex kInvalidInstruction = InstructionIndex(-1);

  // A microarchitecture index that is not used by any existing
  // microarchitecture.
  static constexpr MicroArchitectureIndex kInvalidMicroArchitecture =
      MicroArchitectureIndex(-1);

  // Initializes the Architecture object with the given architecture proto.
  Architecture(std::shared_ptr<const ArchitectureProto> architecture_proto);
  virtual ~Architecture() {}

  // Disallow copy and assign.
  Architecture(const Architecture&) = delete;
  Architecture& operator=(const Architecture&) = delete;

  // ---------------------------------------------------------------------------
  // Microarchitectures
  // ---------------------------------------------------------------------------

  // Returns the number of microarchitectures supported by this Architecture
  // object.
  MicroArchitectureIndex num_microarchitectures() const {
    return MicroArchitectureIndex(
        architecture_proto_->per_microarchitecture_itineraries_size());
  }

  // Returns the ID of the microarchitecture at the given index.
  const std::string& microarchitecture_id(MicroArchitectureIndex index) const {
    return itineraries(index).microarchitecture_id();
  }

  // Returns the itineraries proto for the given microarchitecture. Using an
  // invalid microarchitecture index will cause a CHECK failure.
  const InstructionSetItinerariesProto& itineraries(
      MicroArchitectureIndex index) const {
    DCHECK_GE(index, 0);
    DCHECK_LT(index, num_microarchitectures());
    return architecture_proto_->per_microarchitecture_itineraries(
        index.value());
  }

  // Returns the index of the microarchitecture with the given name, or
  // kInvalidMicroArchitecture if no such microarchitecture is found in this
  // architecture.
  MicroArchitectureIndex GetMicroArchitectureIndex(
      const std::string& microarchitecture_id) const;

  // ---------------------------------------------------------------------------
  // Instruction lookup
  // ---------------------------------------------------------------------------

  // Looks up instructions by their LLVM mnemonic. Returns a list of indices of
  // the instructions with this mnemonic, or an empty list if no such
  // instruction is found.
  std::vector<InstructionIndex> GetInstructionsByLLVMMnemonic(
      const std::string& llvm_mnemonic) const;

  // Returns the list of indices of instructions with the given encoding
  // specification string. Returns an empty list if no such instruction is
  // found.
  // Note that the encoding specifications are not necessarily unique in the
  // instruction set. For example on x86-64, most allow switching between direct
  // and indirect addressing by updating a field of the ModR/M byte. The two
  // versions have the same encoding specification, but they have different
  // latencies and use different execution units, so we list them as two
  // different instructions.
  std::vector<InstructionIndex> GetInstructionIndicesByRawEncodingSpecification(
      const std::string& encoding_specification) const;

  // Returns the ArchitectureProto powering this instruction database.
  const ArchitectureProto& architecture_proto() const {
    return *architecture_proto_;
  }

  // ---------------------------------------------------------------------------
  // Access to the data
  // ---------------------------------------------------------------------------

  // Returns the number of instructions in the ISA.
  InstructionIndex num_instructions() const {
    return InstructionIndex(
        architecture_proto_->instruction_set().instructions_size());
  }

  // Returns the instruction proto at the given index.
  const InstructionProto& instruction(InstructionIndex index) const {
    DCHECK_GE(index, 0);
    DCHECK_LT(index, num_instructions());
    return architecture_proto_->instruction_set().instructions(index.value());
  }

  // Returns an iterable list of instructions of the architecture.
  const ::google::protobuf::RepeatedPtrField<InstructionProto>& instructions()
      const {
    return architecture_proto_->instruction_set().instructions();
  }

  // Returns the itinerary for the given microarchitecture and instruction.
  const ItineraryProto& itinerary(MicroArchitectureIndex microarchitecture,
                                  InstructionIndex instruction) const {
    DCHECK_GE(instruction, 0);
    DCHECK_LT(instruction, num_instructions());
    return itineraries(microarchitecture).itineraries(instruction.value());
  }

  // Returns the first instruction proto from the given list or nullptr, if the
  // list is empty. Assumes that all indices in the list are valid.
  const InstructionProto* GetFirstInstructionOrNull(
      const std::vector<InstructionIndex>& indices) const {
    if (indices.empty()) return nullptr;
    return &instruction(indices.front());
  }

 private:
  // TODO(ondrasej): In most use cases in this class, the list of instructions
  // will contain only a single instruction. Consider using a data structure
  // optimized for this use case.
  using InstructionsByString =
      absl::flat_hash_map<std::string, std::vector<InstructionIndex>>;

  // The architecture proto that contains the instruction data served by this
  // class.
  const std::shared_ptr<const ArchitectureProto> architecture_proto_;

  // Mappings from instruction mnemonics to indices of the corresponding
  // instructions in architecture_proto_.
  InstructionsByString llvm_to_instruction_index_;

  // Instructions in architecture_proto_ indexed by their raw encoding
  // specification.
  InstructionsByString raw_encoding_specification_to_instruction_index_;

  // The list of microarchitecture indices, indexed by their IDs.
  absl::flat_hash_map<std::string, MicroArchitectureIndex>
      microarchitectures_by_id_;
};

}  // namespace exegesis

#endif  // EXEGESIS_BASE_ARCHITECTURE_H_
