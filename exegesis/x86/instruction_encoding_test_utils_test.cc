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

// Contains tests for the disassembly matcher.

#include "exegesis/x86/instruction_encoding_test_utils.h"

#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

using ::testing::StartsWith;
using ::testing::StringMatchResultListener;

constexpr char kEncodingSpecificationProto[] = R"proto(
  legacy_prefixes {
    rex_w_prefix: PREFIX_IS_IGNORED
    operand_size_override_prefix: PREFIX_IS_IGNORED
  }
  opcode: 0x04
  immediate_value_bytes: 1)proto";

TEST(DisassemblesToTest, ExampleFromHeaderFile) {
  DecodedInstruction decoded_instruction;
  decoded_instruction.set_opcode(0x04);
  decoded_instruction.add_immediate_value("\x0a");
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(kEncodingSpecificationProto, "ADD AL, 0xa"));
}

TEST(DisassemblesToMatcherTest, AcceptsEncodingSpecificationProto) {
  DecodedInstruction decoded_instruction;
  decoded_instruction.set_opcode(0x04);
  decoded_instruction.add_immediate_value("\x0a");
  const EncodingSpecification encoding_specification =
      ParseProtoFromStringOrDie<EncodingSpecification>(
          R"proto(legacy_prefixes {
                    rex_w_prefix: PREFIX_IS_IGNORED
                    operand_size_override_prefix: PREFIX_IS_IGNORED
                  }
                  opcode: 0x04
                  immediate_value_bytes: 1)proto");
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(encoding_specification, "ADD AL, 0xa"));
}

TEST(DisassemblesToTest, IsCaseInsensitive) {
  DecodedInstruction decoded_instruction;
  decoded_instruction.set_opcode(0x04);
  decoded_instruction.add_immediate_value("\x0a");
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(kEncodingSpecificationProto, "ADD AL, 0xa"));
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(kEncodingSpecificationProto, "add al, 0xa"));
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(kEncodingSpecificationProto, "AdD Al, 0xA"));
}

TEST(DisassemblesToMatcherTest, ReportsErrorOnInvalidEncodingSpecification) {
  DecodedInstruction decoded_instruction;
  StringMatchResultListener listener;
  DisassemblesToMatcher matcher("foo? bar!", "");
  EXPECT_FALSE(matcher.MatchAndExplain(decoded_instruction, &listener));
  EXPECT_EQ(listener.str(),
            "Could not parse encoding specification:\nfoo? bar!");
}

TEST(DisassemblesToMatcherTest, ReportsErrorIfEncodingFails) {
  DecodedInstruction decoded_instruction;
  StringMatchResultListener listener;
  DisassemblesToMatcher matcher(kEncodingSpecificationProto, "ADD AL, 0xa");
  EXPECT_FALSE(matcher.MatchAndExplain(decoded_instruction, &listener));
  EXPECT_THAT(listener.str(), StartsWith("Could not encode the instruction: "));
}

TEST(DisassemblesToMatcherTest, ReportsErrorIfDisassemblyDoesNotMatch) {
  DecodedInstruction decoded_instruction;
  decoded_instruction.add_immediate_value("\x0a");
  StringMatchResultListener listener;
  DisassemblesToMatcher matcher(kEncodingSpecificationProto, "ADD AX, 0xa");
  EXPECT_FALSE(matcher.MatchAndExplain(decoded_instruction, &listener));
  EXPECT_EQ(listener.str(),
            "The disassembly does not match.\n"
            "Expected: ADD AX, 0xa\n"
            "Actual: add al, 0xa\n"
            "Binary encoding: 04 0A");
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
