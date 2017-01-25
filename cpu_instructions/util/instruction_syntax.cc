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

#include "cpu_instructions/util/instruction_syntax.h"

#include <stddef.h>
#include <algorithm>
#include <vector>

#include "cpu_instructions/proto/instructions.pb.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "strings/string_view.h"
#include "strings/strip.h"

namespace cpu_instructions {

namespace {
bool ContainsPrefix(StringPiece s, const std::vector<StringPiece>& prefixes) {
  for (const auto& prefix : prefixes) {
    if (s.substr(0, prefix.size()) == prefix) {
      return true;
    }
  }
  return false;
}

std::vector<string> SeparateOperandsWithCommas(const string& s) {
  std::vector<string> result;
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
}  // namespace

InstructionFormat ParseAssemblyStringOrDie(const string& code) {
  // The syntax always has the format [prefix] mnemonic op1, op2[, op3].
  // We parse it by first splitting the string by commas; this will separate the
  // mnemonic and the first operand from the other operands. Then we split the
  // mnemonic and the first operand by spaces.
  InstructionFormat proto;
  std::vector<string> parts = SeparateOperandsWithCommas(code);
  CHECK(!parts.empty());
  // Parse the mnemonic and the optional first operand.
  string mnemonic_and_first_operand = parts[0];
  StripWhitespace(&mnemonic_and_first_operand);
  std::replace(mnemonic_and_first_operand.begin(),
               mnemonic_and_first_operand.end(), '\t', ' ');

  CHECK(!mnemonic_and_first_operand.empty());
  const std::vector<StringPiece> kX86Prefixes = {"LOCK", "REP"};
  size_t delimiting_space = mnemonic_and_first_operand.find_first_of(' ');
  if (delimiting_space != string::npos &&
      ContainsPrefix(mnemonic_and_first_operand, kX86Prefixes)) {
    delimiting_space =
        mnemonic_and_first_operand.find_first_of(' ', delimiting_space + 1);
  }
  if (delimiting_space == string::npos) {
    proto.set_mnemonic(mnemonic_and_first_operand);
  } else {
    proto.set_mnemonic(mnemonic_and_first_operand.substr(0, delimiting_space));
    proto.add_operands()->set_name(
        mnemonic_and_first_operand.substr(delimiting_space + 1));
  }

  // Copy the remaining operands.
  for (int i = 1; i < parts.size(); ++i) {
    string operand = parts[i];
    StripWhitespace(&operand);
    proto.add_operands()->set_name(operand);
  }
  return proto;
}

string ConvertToCodeString(const InstructionFormat& instruction) {
  string result = instruction.mnemonic();
  bool run_once = false;
  for (const auto& operand : instruction.operands()) {
    StrAppend(&result, run_once ? "," : " ", operand.name());
    run_once = true;
  }
  return result;
}

}  // namespace cpu_instructions
