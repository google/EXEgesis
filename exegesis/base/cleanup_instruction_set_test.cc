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

#include "exegesis/base/cleanup_instruction_set.h"

#include <functional>

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace {

using ::exegesis::InstructionProto;
using ::exegesis::InstructionSetProto;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::TextFormat;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::error::INVALID_ARGUMENT;

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
        raw_encoding_specification: 'D8 C8+i' })";
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

// A dummy transform that deletes the second instruction in the instruction set,
// and returns Status::OK. Used for testing the diff.
Status DeleteSecondInstruction(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(instructions->begin() + 1);
  return OkStatus();
}

TEST(RunTransformWithDiffTest, WithDifference) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
        encoding_scheme: 'NP'
        raw_encoding_specification: 'AE' }
      instructions {
        vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                        operands { name: 'DX' }}
        encoding_scheme: 'NP' raw_encoding_specification: '6C' }
      instructions {
        vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                        operands { name: 'DX' }}
        encoding_scheme: 'NP' raw_encoding_specification: '6D' })";
  constexpr char kExpectedDiff[] =
      "deleted: instructions[1]: { vendor_syntax { mnemonic: \"INS\" operands "
      "{ name: \"m8\" } operands { name: \"DX\" } } encoding_scheme: \"NP\" "
      "raw_encoding_specification: \"6C\" }\n";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  const StatusOr<string> diff_or_status =
      RunTransformWithDiff(DeleteSecondInstruction, &instruction_set);
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
        raw_encoding_specification: 'D8 C8+i' })";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  const StatusOr<string> diff_or_status =
      RunTransformWithDiff(ReturnErrorInsteadOfTransforming, &instruction_set);
  EXPECT_EQ(diff_or_status.status().error_code(), INVALID_ARGUMENT);
  EXPECT_EQ(diff_or_status.status().error_message(), "I do not transform!");
}

TEST(SortByVendorSyntaxTest, Sort) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'AE' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' raw_encoding_specification: '6C' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' raw_encoding_specification: '6D' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm16' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' raw_encoding_specification: '6D' }
         instructions {
           vendor_syntax { mnemonic: 'INS' operands { name: 'm8' }
                           operands { name: 'DX' }}
           encoding_scheme: 'NP' raw_encoding_specification: '6C' }
         instructions {
           vendor_syntax { mnemonic: 'SCAS' operands { name: 'm8' }}
           encoding_scheme: 'NP'
           raw_encoding_specification: 'AE' })";
  TestTransform(SortByVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace exegesis
