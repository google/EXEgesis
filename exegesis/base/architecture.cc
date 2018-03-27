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

#include "exegesis/base/architecture.h"

#include <utility>
#include <vector>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {

constexpr Architecture::InstructionIndex Architecture::kInvalidInstruction;
constexpr Architecture::MicroArchitectureIndex
    Architecture::kInvalidMicroArchitecture;

Architecture::Architecture(
    std::shared_ptr<const ArchitectureProto> architecture_proto)
    : architecture_proto_(std::move(architecture_proto)) {
  CHECK(architecture_proto_ != nullptr);
  for (InstructionIndex index(0); index < num_instructions(); ++index) {
    const InstructionProto& instruction_proto = instruction(index);
    const std::string& encoding_specification =
        instruction_proto.raw_encoding_specification();
    raw_encoding_specification_to_instruction_index_[encoding_specification]
        .push_back(index);

    const std::string& llvm_mnemonic = instruction_proto.llvm_mnemonic();
    if (!llvm_mnemonic.empty()) {
      llvm_to_instruction_index_[llvm_mnemonic].push_back(index);
    } else {
      VLOG(1) << "Missing llvm mnemonic for instruction at position " << index
              << std::endl
              << instruction_proto.DebugString();
    }
  }

  for (MicroArchitectureIndex index(0); index < num_microarchitectures();
       ++index) {
    microarchitectures_by_id_[microarchitecture_id(index)] = index;
  }
}

Architecture::MicroArchitectureIndex Architecture::GetMicroArchitectureIndex(
    const std::string& microarchitecture_id) const {
  return FindWithDefault(microarchitectures_by_id_, microarchitecture_id,
                         kInvalidMicroArchitecture);
}

std::vector<Architecture::InstructionIndex>
Architecture::GetInstructionsByLLVMMnemonic(
    const std::string& llvm_mnemonic) const {
  return FindWithDefault(llvm_to_instruction_index_, llvm_mnemonic, {});
}

std::vector<Architecture::InstructionIndex>
Architecture::GetInstructionIndicesByRawEncodingSpecification(
    const std::string& encoding_specification) const {
  return FindWithDefault(raw_encoding_specification_to_instruction_index_,
                         encoding_specification, {});
}

}  // namespace exegesis
