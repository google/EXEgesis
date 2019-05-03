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

#ifndef THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_
#define THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_

#include <string>

namespace exegesis {

// Remove syllabification.
//
// eg.
// "Here is a para-
// graph with an hypen." -> "Here is a paragraph with an hypen."
std::string Dehyphenate(std::string input);
void DehyphenateinPlace(std::string* input);

// Remove leading and trailing whitespace in a multiline string.
std::string RemoveLineLeadingTrailingWhitespace(std::string input);
void RemoveLineLeadingTrailingWhitespaceInPlace(std::string* input);

// Transforms \r\n into \n
std::string CleanupLineFeed(std::string input);
void CleanupLineFeedInPlace(std::string* input);

// Keep a maximum of two consecutive \n.
std::string CondenseLineFeeds(std::string input);
void CondenseLineFeedsInPlace(std::string* input);

// Unfolds a paragraph by joining all lines and removing hyphen when necessary.
// If a line ends with a dot, the next line will not be joined.
// It also removes leading/trailing whitespace and normalizes line feed.
//
// eg.
// "This is a single line.
//
// This is a paragraph that
// wraps and even has hu-
// mon-
// gous words with -." -> "This is a single line.
//
// This is a paragraph that wraps and even has humongous words with -."
std::string CleanupParagraph(std::string input);

}  // namespace exegesis

#endif  // THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_
