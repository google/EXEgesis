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

#include "exegesis/x86/instruction_encoding_test_utils.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/llvm/assembler_disassembler.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/strings.h"
#include "exegesis/x86/encoding_specification.h"
#include "exegesis/x86/instruction_encoder.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::StatusOr;
using ::testing::MakeMatcher;
using ::testing::Matcher;
using ::testing::MatchResultListener;

inline bool EqualsIgnoreCase(absl::string_view a, absl::string_view b) {
  return a.size() == b.size() && absl::StartsWithIgnoreCase(a, b);
}

DisassemblesToMatcher::DisassemblesToMatcher(
    const std::string& encoding_specification_proto,
    std::string expected_disassembly)
    : expected_disassembly_(std::move(expected_disassembly)) {
  // We could CHECK that the parsing was successful, but that would kill the
  // test if there is a syntax error in the encoding specification. Instead, we
  // just report a match failure with an appropriate error message in
  // MatchAndExplain, and let the test process continue;
  if (!::google::protobuf::TextFormat::ParseFromString(
          encoding_specification_proto, &encoding_specification_)) {
    encoding_specification_str_ = encoding_specification_proto;
  }
}

DisassemblesToMatcher::DisassemblesToMatcher(
    const EncodingSpecification& encoding_specification,
    std::string expected_disassembly)
    : encoding_specification_(encoding_specification),
      expected_disassembly_(std::move(expected_disassembly)) {}

bool DisassemblesToMatcher::MatchAndExplain(
    const DecodedInstruction& decoded_instruction,
    MatchResultListener* listener) const {
  // The encoding specification was not parsed correctly. Report this as a match
  // error.
  if (!encoding_specification_str_.empty()) {
    if (listener->IsInterested()) {
      *listener << "Could not parse encoding specification:\n"
                << encoding_specification_str_;
    }
    return false;
  }

  const StatusOr<std::vector<uint8_t>> encoded_instruction_or_status =
      EncodeInstruction(encoding_specification_, decoded_instruction);
  if (!encoded_instruction_or_status.ok()) {
    if (listener->IsInterested()) {
      *listener << "Could not encode the instruction: "
                << encoded_instruction_or_status.status();
    }
    return false;
  }
  const std::vector<uint8_t>& encoded_instruction =
      encoded_instruction_or_status.ValueOrDie();

  // TODO(ondrasej): If creating the disassembler is too costly, consider
  // allocating only one and then keeping the pointer until the end of the
  // program.
  AssemblerDisassembler asm_disasm;
  const auto result = asm_disasm.Disassemble(encoded_instruction);
  if (!result.status().ok()) {
    if (listener->IsInterested()) {
      *listener << "Could not disassemble the instruction.";
    }
    return false;
  }

  const std::string actual_disassembly =
      ConvertToCodeString(result.ValueOrDie().intel_syntax());
  const bool is_match =
      EqualsIgnoreCase(actual_disassembly, expected_disassembly_);
  if (!is_match) {
    if (listener->IsInterested()) {
      *listener << "The disassembly does not match.\n"
                << "Expected: " << expected_disassembly_ << "\n"
                << "Actual: " << actual_disassembly << "\n"
                << "Binary encoding: "
                << ToHumanReadableHexString(encoded_instruction);
    }
  }
  return is_match;
}

::testing::Matcher<const DecodedInstruction&> DisassemblesTo(
    const std::string& encoding_specification_proto,
    const std::string& expected_disassembly) {
  return ::testing::MakeMatcher(new DisassemblesToMatcher(
      encoding_specification_proto, expected_disassembly));
}

::testing::Matcher<const DecodedInstruction&> DisassemblesTo(
    const EncodingSpecification& encoding_specification,
    const std::string& expected_disassembly) {
  return ::testing::MakeMatcher(
      new DisassemblesToMatcher(encoding_specification, expected_disassembly));
}

}  // namespace x86
}  // namespace exegesis
