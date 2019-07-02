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

#include "base/casts.h"
#include "base/commandlineflags.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace {

// This printer ensures instruction_group_index is printed even if it has value
// of 0.
class InstructionGroupIndexPrinter
    : public google::protobuf::TextFormat::FastFieldValuePrinter {
 public:
  ~InstructionGroupIndexPrinter() override {}

  void PrintMessageEnd(const google::protobuf::Message& message,
                       int field_index, int field_count, bool single_line_mode,
                       google::protobuf::TextFormat::BaseTextGenerator*
                           generator) const override {
    if (message.GetDescriptor() == InstructionProto::descriptor() &&
        down_cast<const InstructionProto&>(message).instruction_group_index() ==
            0) {
      generator->Indent();
      generator->PrintString("instruction_group_index: 0\n");
      generator->Outdent();
    }

    google::protobuf::TextFormat::FastFieldValuePrinter::PrintMessageEnd(
        message, field_index, field_count, single_line_mode, generator);
  }
};

}  // namespace

constexpr Architecture::InstructionIndex Architecture::kInvalidInstruction;
constexpr Architecture::MicroArchitectureIndex
    Architecture::kInvalidMicroArchitecture;

std::unique_ptr<google::protobuf::TextFormat::Printer>
GetArchitectureProtoTextPrinter() {
  std::unique_ptr<google::protobuf::TextFormat::Printer> printer =
      absl::make_unique<google::protobuf::TextFormat::Printer>();
  printer->SetExpandAny(true);
  printer->SetPrintMessageFieldsInIndexOrder(true);
  printer->SetDefaultFieldValuePrinter(new InstructionGroupIndexPrinter());
  return printer;
}

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
  return ::exegesis::gtl::FindWithDefault(microarchitectures_by_id_,
                                          microarchitecture_id,
                                          kInvalidMicroArchitecture);
}

std::vector<Architecture::InstructionIndex>
Architecture::GetInstructionsByLLVMMnemonic(
    const std::string& llvm_mnemonic) const {
  return ::exegesis::gtl::FindWithDefault(llvm_to_instruction_index_,
                                          llvm_mnemonic, {});
}

std::vector<Architecture::InstructionIndex>
Architecture::GetInstructionIndicesByRawEncodingSpecification(
    const std::string& encoding_specification) const {
  return ::exegesis::gtl::FindWithDefault(
      raw_encoding_specification_to_instruction_index_, encoding_specification,
      {});
}

}  // namespace exegesis
