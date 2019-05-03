// Copyright 2017 Google Inc.
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

#include "exegesis/util/text_processing.h"

#include "re2/re2.h"

namespace exegesis {

void DehyphenateInPlace(std::string* input) {
  // We need to match characters before hypen so we don't join items in lists.
  static const LazyRE2 re = {R"((\pL)-\r?\n(\pL))"};
  RE2::GlobalReplace(input, *re, R"(\1\2)");
}

std::string Dehyphenate(std::string input) {
  DehyphenateInPlace(&input);
  return input;
}

void RemoveLineLeadingTrailingWhitespaceInPlace(std::string* input) {
  static const LazyRE2 leading_whitespace = {R"((?m:^([ \t\p{Zs}]+)))"};
  RE2::GlobalReplace(input, *leading_whitespace, "");

  static const LazyRE2 trailing_whitespace = {R"((?m:([ \t\p{Zs}]+)$))"};
  RE2::GlobalReplace(input, *trailing_whitespace, "");
}

std::string RemoveLineLeadingTrailingWhitespace(std::string input) {
  RemoveLineLeadingTrailingWhitespaceInPlace(&input);
  return input;
}

void CleanupLineFeedInPlace(std::string* input) {
  static const LazyRE2 cleanup_line_feed = {R"((\r?\n))"};
  RE2::GlobalReplace(input, *cleanup_line_feed, "\n");
}

std::string CleanupLineFeed(std::string input) {
  CleanupLineFeedInPlace(&input);
  return input;
}

void CondenseLineFeedsInPlace(std::string* input) {
  static const LazyRE2 condense_line_feed = {R"((\n){3,})"};
  RE2::GlobalReplace(input, *condense_line_feed, "\n\n");
}

std::string CondenseLineFeeds(std::string input) {
  CondenseLineFeedsInPlace(&input);
  return input;
}

std::string CleanupParagraph(std::string input) {
  RemoveLineLeadingTrailingWhitespaceInPlace(&input);
  CleanupLineFeedInPlace(&input);
  CondenseLineFeedsInPlace(&input);
  DehyphenateInPlace(&input);
  static const LazyRE2 re = {R"(([^.\n])\n([^-\n]))"};
  RE2::GlobalReplace(&input, *re, R"(\1 \2)");
  return input;
}

}  // namespace exegesis
