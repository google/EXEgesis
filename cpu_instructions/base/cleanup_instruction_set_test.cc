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

#include "cpu_instructions/base/cleanup_instruction_set.h"

#include <functional>

#include "src/google/protobuf/text_format.h"
#include "gmock/gmock.h"
#include "cpu_instructions/testing/test_util.h"
#include "gtest/gtest.h"
#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace {

using ::cpu_instructions::InstructionFormat;
using ::cpu_instructions::InstructionOperand;
using ::cpu_instructions::InstructionProto;
using ::cpu_instructions::InstructionSetProto;
using ::google::protobuf::TextFormat;
using ::cpu_instructions::testing::EqualsProto;
using ::cpu_instructions::util::InvalidArgumentError;
using ::cpu_instructions::util::Status;
using ::cpu_instructions::util::error::INVALID_ARGUMENT;

TEST(GetTransformsByNameTest, ReturnedMapIsNotEmpty) {
  const InstructionSetTransformsByName& transforms = GetTransformsByName();
  EXPECT_GT(transforms.size(), 0);
}

TEST(GetDefaultTransformPipelineTest, ReturnedVectorIsNotEmpty) {
  const std::vector<InstructionSetTransform> transforms =
      GetDefaultTransformPipeline();
  EXPECT_GT(transforms.size(), 0);
}

TEST(RunTransformWithDiffTest, NoDifference) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: 'FMUL'
          operands { name: 'ST(0)' } operands { name: 'ST(i)' }}
        feature_name: 'X87'
        binary_encoding: 'D8 C8+i' })";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  // There is only one instruction in the proto, so SortByVendorSyntax leaves
  // the proto unchanged.
  const StatusOr<string> diff_or_status =
      RunTransformWithDiff(SortByVendorSyntax, &instruction_set);
  ASSERT_OK(diff_or_status.status());
  EXPECT_EQ(diff_or_status.ValueOrDie(), "");
}

// A dummy transform that clears the binary encoding of the instruction, and
// returns Status::OK, used for testing the diff.
Status ClearBinaryEncoding(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    instruction.set_binary_encoding("");
  }
  return Status::OK;
}

TEST(RunTransformWithDiffTest, WithDifference) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: 'FMUL'
          operands { name: 'ST(0)' } operands { name: 'ST(i)' }}
        feature_name: 'X87'
        binary_encoding: 'D8 C8+i' })";
  constexpr char kExpectedDiff[] =
      "added: instructions[0]: { vendor_syntax { mnemonic: \"FMUL\" operands { "
      "name: \"ST(0)\" } operands { name: \"ST(i)\" } } feature_name: \"X87\" "
      "binary_encoding: \"\" }\ndeleted: instructions[0]: { vendor_syntax { "
      "mnemonic: \"FMUL\" operands { name: \"ST(0)\" } operands { name: "
      "\"ST(i)\" } } feature_name: \"X87\" binary_encoding: \"D8 C8+i\" }\n";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  const StatusOr<string> diff_or_status =
      RunTransformWithDiff(ClearBinaryEncoding, &instruction_set);
  ASSERT_OK(diff_or_status.status());
  EXPECT_EQ(diff_or_status.ValueOrDie(), kExpectedDiff);
}

// A dummy transform that immediately returns an error.
Status ReturnErrorInsteadOfTransforming(InstructionSetProto* instruction_set) {
  return InvalidArgumentError("I do not transform!");
}

TEST(RunTransformWithDiffTest, WithError) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: 'FMUL'
          operands { name: 'ST(0)' } operands { name: 'ST(i)' }}
        feature_name: 'X87'
        binary_encoding: 'D8 C8+i' })";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  const StatusOr<string> diff_or_status =
      RunTransformWithDiff(ReturnErrorInsteadOfTransforming, &instruction_set);
  EXPECT_EQ(diff_or_status.status().error_code(), INVALID_ARGUMENT);
  EXPECT_EQ(diff_or_status.status().error_message(), "I do not transform!");
}

TEST(SortByVendorSyntaxTest, Sort) {
  static const char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           binary_encoding: 'AE' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6C' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6D' })";
  static const char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6D' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' binary_encoding: '6C' }
         instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           binary_encoding: 'AE' })";
  TestTransform(SortByVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace cpu_instructions
