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

#include "exegesis/util/instruction_syntax.h"

#include <stddef.h>
#include <algorithm>
#include <vector>

#include "exegesis/proto/instructions.pb.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "strings/string_view.h"
#include "strings/string_view_utils.h"
#include "strings/strip.h"

namespace exegesis {
namespace {

// RE2 and protobuf have two slightly different implementations of StringPiece.
// In this file, we use RE2::Consume, so we explicitly switch to the RE2
// implementation.
using ::re2::StringPiece;

template <typename PrefixCollection>
bool ContainsPrefix(const std::string& str, const PrefixCollection& prefixes) {
  for (const auto& prefix : prefixes) {
    if (strings::StartsWith(str, prefix)) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> SeparateOperandsWithCommas(const std::string& s) {
  std::vector<std::string> result;
  bool in_parenthesis = false;
  int start = 0;
  for (int i = 0; i < s.size(); ++i) {
    const char& c = s[i];
    if (c == '(') {
      in_parenthesis = true;
    } else if (c == ')') {
      in_parenthesis = false;
    } else if (c == ',' && !in_parenthesis) {
      result.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  }
  result.push_back(s.substr(start, s.size() - start));
  return result;
}

InstructionOperand ParseOperand(StringPiece source) {
  StringPiece original_source = source;
  static const LazyRE2 kOperandNameRegexp = {R"( *([^{}]*[^{} ]) *)"};
  static const LazyRE2 kTagRegexp = {R"( *\{([^}]+)\} *)"};
  InstructionOperand operand;
  // In the assembly syntax for AVX-512 features introduced in the Intel SDM,
  // some tags ({*-sae} and {sae}) are separated from the other operands by a
  // comma, even though there is no "real" operand. We parse this as an
  // InstructionOperand with a list of tags and an empty name.
  // Later, we CHECK() that at least one of the following holds:
  // 1. The operand has a non-empty name.
  // 2. The operand has one or more tags.
  RE2::Consume(&source, *kOperandNameRegexp, operand.mutable_name());
  while (!source.empty()) {
    CHECK(
        RE2::Consume(&source, *kTagRegexp, operand.add_tags()->mutable_name()))
        << "Remaining source: \"" << source << "\"";
  }
  CHECK(!operand.name().empty() || operand.tags_size() > 0)
      << "Neither operand name nor any tags were found, source = \""
      << original_source << "\"";
  return operand;
}

}  // namespace

InstructionFormat ParseAssemblyStringOrDie(const std::string& code) {
  // The syntax always has the format [prefix] mnemonic op1, op2[, op3].
  // We parse it by first splitting the string by commas; this will separate the
  // mnemonic and the first operand from the other operands. Then we split the
  // mnemonic and the first operand by spaces.
  InstructionFormat proto;
  std::vector<std::string> parts = SeparateOperandsWithCommas(code);
  CHECK(!parts.empty());
  // Parse the mnemonic and the optional first operand.
  std::string mnemonic_and_first_operand = parts[0];
  StripWhitespace(&mnemonic_and_first_operand);
  std::replace(mnemonic_and_first_operand.begin(),
               mnemonic_and_first_operand.end(), '\t', ' ');

  CHECK(!mnemonic_and_first_operand.empty());
  constexpr const char* kX86Prefixes[] = {"LOCK", "REP"};
  size_t delimiting_space = mnemonic_and_first_operand.find_first_of(' ');
  if (delimiting_space != std::string::npos &&
      ContainsPrefix(mnemonic_and_first_operand, kX86Prefixes)) {
    delimiting_space =
        mnemonic_and_first_operand.find_first_of(' ', delimiting_space + 1);
  }
  if (delimiting_space == std::string::npos) {
    proto.set_mnemonic(mnemonic_and_first_operand);
  } else {
    proto.set_mnemonic(mnemonic_and_first_operand.substr(0, delimiting_space));
    *proto.add_operands() =
        ParseOperand(mnemonic_and_first_operand.substr(delimiting_space + 1));
  }

  // Copy the remaining operands.
  for (int i = 1; i < parts.size(); ++i) {
    *proto.add_operands() = ParseOperand(parts[i]);
  }
  return proto;
}

std::string ConvertToCodeString(const InstructionFormat& instruction) {
  std::string result = instruction.mnemonic();
  bool run_once = false;
  for (const auto& operand : instruction.operands()) {
    StrAppend(&result, run_once ? ", " : " ", operand.name());
    for (const InstructionOperand::Tag& tag : operand.tags()) {
      if (!result.empty() && result.back() != ' ') result += ' ';
      StrAppend(&result, "{", tag.name(), "}");
    }
    run_once = true;
  }
  return result;
}

}  // namespace exegesis
