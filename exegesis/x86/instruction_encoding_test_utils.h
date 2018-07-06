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

#ifndef EXEGESIS_X86_INSTRUCTION_ENCODING_TEST_UTILS_H_
#define EXEGESIS_X86_INSTRUCTION_ENCODING_TEST_UTILS_H_

#include <memory>
#include <ostream>
#include <string>

#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "gmock/gmock.h"

namespace exegesis {
namespace x86 {

// A gMock matcher that encodes an instruction in the form of the
// DecodedInstruction proto using encoding_specification as the encoding
// specification, disassembles the binary encoding using the LLVM disassembler
// and verifies that the disassembly matches expected_disassembly.
//
// The matcher is designed to be very robust, and it reports a failure (rather
// than CHECK-fail) even if the encoding specification is not valid or if the
// instruction can't be encoded in the first place.
//
// Usage:
// constexpr char kEncodingSpecificationProto[] = R"proto(
//   legacy_prefixes { rex_w_prefix: PREFIX_IS_IGNORED }
//   opcode: 0x04
//   immediate_value_bytes: 1)proto";
// DecodedInstruction instruction_data;
// instruction_data.set_opcode(0x04);
// instruction_data.add_immediate_values("\x0a");
// EXPECT_THAT(instruction_data,
//             DisassemblesTo(kEncodingSpecificationProto, "ADD AL, 0xa"));
::testing::Matcher<const DecodedInstruction&> DisassemblesTo(
    const std::string& encoding_specification_proto,
    const std::string& expected_disassembly);
::testing::Matcher<const DecodedInstruction&> DisassemblesTo(
    const EncodingSpecification& encoding_specification_proto,
    const std::string& expected_disassembly);

// -----------------------------------------------------------------------------
//  The following code is internal to the library, and it is exposed only for
//  testing.
// -----------------------------------------------------------------------------

// The implementation of the matcher.
class DisassemblesToMatcher
    : public ::testing::MatcherInterface<const DecodedInstruction&> {
 public:
  DisassemblesToMatcher(const std::string& encoding_specification_proto,
                        std::string expected_disassembly);
  DisassemblesToMatcher(const EncodingSpecification& specification,
                        std::string expected_disassembly);

  bool MatchAndExplain(const DecodedInstruction& instruction_data,
                       ::testing::MatchResultListener* listener) const override;

  void DescribeTo(std::ostream* os) const override {
    *os << "disassembles to " << expected_disassembly_;
  }
  void DescribeNegationTo(std::ostream* os) const override {
    *os << "does not disassemble to " << expected_disassembly_;
  }

 private:
  // The parsed instruction encoding specification; if the specification can't
  // be parsed, this field will contain nullptr, and encoding_specification_str_
  // will be set so that the failure can be reported by MatchAndExplain.
  EncodingSpecification encoding_specification_;

  // The string version of the instruction encoding specification. This field is
  // used only for reporting errors, and it is set only if the specification
  // can't be parsed.
  std::string encoding_specification_str_;

  // The expected disassembly entered by the user.
  const std::string expected_disassembly_;
};

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_INSTRUCTION_ENCODING_TEST_UTILS_H_
