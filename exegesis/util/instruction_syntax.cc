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

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/proto/instructions.pb.h"
#include "glog/logging.h"
#include "re2/re2.h"

namespace exegesis {
namespace {

template <typename PrefixCollection>
bool ContainsPrefix(const std::string& str, const PrefixCollection& prefixes) {
  for (const auto& prefix : prefixes) {
    if (absl::StartsWith(str, prefix)) {
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

InstructionOperand ParseOperand(absl::string_view source) {
  absl::string_view original_source = source;
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
  absl::StripAsciiWhitespace(&mnemonic_and_first_operand);
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
    absl::StrAppend(&result, run_once ? ", " : " ", operand.name());
    for (const InstructionOperand::Tag& tag : operand.tags()) {
      if (!result.empty() && result.back() != ' ') result += ' ';
      absl::StrAppend(&result, "{", tag.name(), "}");
    }
    run_once = true;
  }
  return result;
}

InstructionFormat* GetOrAddUniqueVendorSyntaxOrDie(
    InstructionProto* instruction) {
  CHECK(instruction != nullptr);
  const int num_vendor_syntaxes = instruction->vendor_syntax_size();
  CHECK_LE(num_vendor_syntaxes, 1);
  return num_vendor_syntaxes == 0 ? instruction->add_vendor_syntax()
                                  : instruction->mutable_vendor_syntax(0);
}

const InstructionFormat& GetVendorSyntaxWithMostOperandsOrDie(
    const InstructionProto& instruction) {
  CHECK_GT(instruction.vendor_syntax_size(), 0);
  const InstructionFormat* best_syntax = nullptr;
  int best_num_operands = -1;
  for (const InstructionFormat& syntax : instruction.vendor_syntax()) {
    if (syntax.operands_size() > best_num_operands) {
      best_syntax = &syntax;
      best_num_operands = syntax.operands_size();
    }
  }
  return *best_syntax;
}

bool HasMnemonicInVendorSyntax(const InstructionProto& instruction,
                               absl::string_view mnemonic) {
  for (const InstructionFormat& vendor_syntax : instruction.vendor_syntax()) {
    if (vendor_syntax.mnemonic() == mnemonic) return true;
  }
  return false;
}

}  // namespace exegesis
