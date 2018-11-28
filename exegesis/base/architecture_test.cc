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

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;
using ::testing::Contains;
using ::testing::HasSubstr;

const char kArchitectureProto[] = R"proto(
  instruction_set {
    instructions {
      llvm_mnemonic: 'ADD8rm'
      att_syntax { mnemonic: 'addb' }
      vendor_syntax { mnemonic: 'ADD' }
      syntax { mnemonic: 'ADD' }
      feature_name: ''
      raw_encoding_specification: '80 /0 ib'
      encoding_scheme: 'MI'
      description: 'Add imm8 to r8/m8.'
    }
    instructions {
      llvm_mnemonic: 'MOV8rr'
      att_syntax { mnemonic: 'movb' }
      vendor_syntax { mnemonic: 'MOV' }
      syntax { mnemonic: 'MOV' }
      feature_name: ''
      raw_encoding_specification: '88 /r'
      encoding_scheme: 'MR'
      description: 'Move r8 to r8/m8. '
    }
    instructions {
      llvm_mnemonic: 'MOV8rm'
      att_syntax { mnemonic: 'movb' }
      vendor_syntax { mnemonic: 'MOV' }
      syntax { mnemonic: 'MOV' }
      feature_name: ''
      raw_encoding_specification: '88 /r'
      encoding_scheme: 'MR'
      description: 'Move r8 to r8/m8. '
    }
    instructions {
      llvm_mnemonic: 'MOV16rr'
      att_syntax { mnemonic: 'movw' }
      syntax { mnemonic: 'MOV' }
      vendor_syntax { mnemonic: 'MOV' }
      feature_name: ''
      raw_encoding_specification: '89 /r'
      encoding_scheme: 'MR'
      description: 'Move r16 to r16/m16.'
    }
  })proto";

bool CompareProtosByLLVMMnemonic(const InstructionProto& left,
                                 const InstructionProto& right) {
  return left.llvm_mnemonic() < right.llvm_mnemonic();
}

class ArchitectureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    architecture_proto_ = std::make_shared<ArchitectureProto>();
    ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(
        kArchitectureProto, architecture_proto_.get()));
    architecture_ = absl::make_unique<Architecture>(architecture_proto_);
  }

  std::shared_ptr<ArchitectureProto> architecture_proto_;
  std::unique_ptr<Architecture> architecture_;
};

TEST_F(ArchitectureTest, TestArchitectureProto) {
  EXPECT_THAT(architecture_->architecture_proto(),
              EqualsProto(*architecture_proto_));
}

TEST_F(ArchitectureTest, TestGetInstructionByLLVMMnemonicOrNull) {
  using InstructionIndex = Architecture::InstructionIndex;
  const struct {
    const char* mnemonic;
    std::vector<Architecture::InstructionIndex> expected_indices;
  } kTestCases[] = {{"ADD8rm", {InstructionIndex(0)}},
                    {"MOV8rm", {InstructionIndex(2)}},
                    {"CMOV32rr", {}}};
  for (const auto& test_case : kTestCases) {
    const std::string mnemonic = test_case.mnemonic;
    SCOPED_TRACE(absl::StrCat("Mnemonic ", mnemonic));
    std::vector<InstructionProto> expected_instructions;
    for (const InstructionIndex index : test_case.expected_indices) {
      expected_instructions.push_back(
          architecture_proto_->instruction_set().instructions(index.value()));
    }
    std::sort(expected_instructions.begin(), expected_instructions.end(),
              CompareProtosByLLVMMnemonic);

    std::vector<InstructionProto> instructions;
    for (const Architecture::InstructionIndex index :
         architecture_->GetInstructionsByLLVMMnemonic(mnemonic)) {
      instructions.push_back(architecture_->instruction(index));
    }
    std::sort(instructions.begin(), instructions.end(),
              CompareProtosByLLVMMnemonic);

    ASSERT_EQ(expected_instructions.size(), instructions.size());
    for (int j = 0; j < instructions.size(); ++j) {
      EXPECT_THAT(instructions[j], EqualsProto(expected_instructions[j]));
    }
  }
}

// Checks that for reach instruction:
// 1. GetInstructionIndicesByRawEncodingSpecification() returns its own index
//    when searching for its raw encoding specification,
// 2. all instructions returned by
//    GetInstructionIndicesByRawEncodingSpecification() have the same raw
//    encoding specification.
TEST_F(ArchitectureTest, GetInstructionIndicesByRawEncodingSpecification) {
  for (Architecture::InstructionIndex instruction_index(0);
       instruction_index < architecture_->num_instructions();
       ++instruction_index) {
    const InstructionProto& instruction =
        architecture_->instruction(instruction_index);
    EXPECT_FALSE(instruction.raw_encoding_specification().empty());
    const std::vector<Architecture::InstructionIndex> indices =
        architecture_->GetInstructionIndicesByRawEncodingSpecification(
            instruction.raw_encoding_specification());
    EXPECT_THAT(indices, Contains(instruction_index));
    for (const Architecture::InstructionIndex other_index : indices) {
      const InstructionProto& other_instruction =
          architecture_->instruction(other_index);
      EXPECT_EQ(other_instruction.raw_encoding_specification(),
                instruction.raw_encoding_specification());
    }
  }
}

TEST(PrintInstructionSetProtoProtoTest, PrintZeroIndexedGroup) {
  constexpr char kArchitectureProto[] = R"proto(
    instructions {
      description: "Enter VMX root operation."
      vendor_syntax {
        mnemonic: "VMXON"
        operands { name: "m64" }
      }
      raw_encoding_specification: "F3 0F C7 /6"
      instruction_group_index: 0
    })proto";
  const InstructionSetProto proto =
      ParseProtoFromStringOrDie<InstructionSetProto>(kArchitectureProto);

  auto printer = GetArchitectureProtoTextPrinter();
  std::string printed_proto_text;
  ASSERT_TRUE(printer->PrintToString(proto, &printed_proto_text));
  EXPECT_THAT(proto, EqualsProto(printed_proto_text));
  EXPECT_THAT(printed_proto_text, HasSubstr("instruction_group_index: 0"));
}

}  // namespace
}  // namespace exegesis
