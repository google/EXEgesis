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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(TextProcessingTest, Dehyphenate) {
  EXPECT_THAT(Dehyphenate("Here is a para-\ngraph with an -."),
              "Here is a paragraph with an -.");
}

TEST(TextProcessingTest, CleanupParagraph) {
  EXPECT_THAT(CleanupParagraph("This is a single line.\n\nThis is a paragraph "
                               "that\nwraps and even has hu-\nmon-\ngous words "
                               "with -."),
              "This is a single line.\n\nThis is a paragraph that wraps and "
              "even has humongous words with -.");
}

TEST(TextProcessingTest, CleanupParagraphSpaces) {
  EXPECT_THAT(CleanupParagraph("Line ends with space \nand continues"),
              "Line ends with space and continues");
}

TEST(TextProcessingTest, CleanupParagraphBracketAndParenthesis) {
  EXPECT_THAT(CleanupParagraph("Line ends with [bracket] \nand continues"),
              "Line ends with [bracket] and continues");
  EXPECT_THAT(CleanupParagraph("Line ends with (parens)\nand continues"),
              "Line ends with (parens) and continues");
  EXPECT_THAT(CleanupParagraph("Line ends with a comma,\nand continues"),
              "Line ends with a comma, and continues");
}

TEST(TextProcessingTest, CleanupParagraphAmpersand) {
  EXPECT_THAT(CleanupParagraph("Line ends with &\nand continues"),
              "Line ends with & and continues");
  EXPECT_THAT(CleanupParagraph("Line ends with\n& and continues"),
              "Line ends with & and continues");
}

TEST(TextProcessingTest, CleanupParagraphCommas) {
  EXPECT_THAT(CleanupParagraph("CR0,CR2,\nCR3"), "CR0,CR2, CR3");
}

TEST(TextProcessingTest, CleanupParagraphWithList) {
  const char list_of_items[] = "Some items\n- 1\n- 2";
  EXPECT_THAT(CleanupParagraph(list_of_items), list_of_items);
}

TEST(TextProcessingTest, RemoveLineLeadingTrailingWhitespace) {
  EXPECT_THAT(RemoveLineLeadingTrailingWhitespace("a   \nb  \nc "), "a\nb\nc");
  EXPECT_THAT(RemoveLineLeadingTrailingWhitespace("   a\n  b\n c"), "a\nb\nc");
  EXPECT_THAT(RemoveLineLeadingTrailingWhitespace("  a \n b \nc "), "a\nb\nc");
}

TEST(TextProcessingTest, CleanupLineFeed) {
  EXPECT_THAT(CleanupLineFeed("a\r\nb\nc"), "a\nb\nc");
}

TEST(TextProcessingTest, CondenseLineFeeds) {
  EXPECT_THAT(CondenseLineFeeds("a\n\n\n\n\nb\nc"), "a\n\nb\nc");
}

}  // namespace
}  // namespace exegesis
