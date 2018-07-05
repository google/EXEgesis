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

#include "exegesis/util/pdf/pdf_document_parser.h"

#include <iterator>

#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

using ::testing::ElementsAreArray;

namespace exegesis {
namespace pdf {

bool operator==(const BoundingBox& a, const BoundingBox& b) {
  return a.left() == b.left() && a.right() == b.right() && a.top() == b.top() &&
         a.bottom() == b.bottom();
}

namespace {

TEST(ExtractLine, no_segment) {
  PdfPage page;
  Cluster(&page);
  ASSERT_EQ(page.segments().size(), 0);
}

TEST(ExtractLine, one_segment) {
  PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"proto(
    characters {
      codepoint: 0x00000049
      utf8: "I"
      font_size: 24.0
      orientation: EAST
      bounding_box: { left: 202.92 top: 165.84 right: 209.328 bottom: 189.84 }
    })proto");
  Cluster(&page);
  ASSERT_EQ(page.segments().size(), 1);
  EXPECT_THAT(page.segments(0).character_indices(), ElementsAreArray({0}));
  ASSERT_EQ(page.rows().size(), 1);
  ASSERT_EQ(page.rows(0).blocks().size(), 1);
  EXPECT_EQ(page.rows(0).blocks(0).text(), "I");
}

TEST(ExtractLine, connect_forward) {
  PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"proto(
    number: 1
    width: 612
    height: 792
    characters: {
      codepoint: 0x00000049
      utf8: "I"
      font_size: 24.0
      orientation: EAST
      bounding_box: { left: 202.92 top: 165.84 right: 209.328 bottom: 189.84 }
      fill_color_hash: 1
    }
    characters: {
      codepoint: 0x0000006e
      utf8: "n"
      font_size: 24.0
      orientation: EAST
      bounding_box: {
        left: 209.3232
        top: 165.84
        right: 223.0992
        bottom: 189.84
      }
      fill_color_hash: 1
    }
  )proto");
  Cluster(&page);
  ASSERT_EQ(page.segments().size(), 1);
  ASSERT_THAT(page.segments(0).character_indices(), ElementsAreArray({0, 1}));
}

TEST(ExtractLine, connect_bottom_top) {
  PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"proto(
    number: 1
    width: 612
    height: 792
    characters: {
      codepoint: 0x00000052
      utf8: "R"
      font_size: 9.0
      orientation: NORTH
      bounding_box: {
        left: 133.08
        top: 153.0869
        right: 142.08
        bottom: 158.8199
      }
      fill_color_hash: 1
    }
    characters: {
      codepoint: 0x00000065
      utf8: "e"
      font_size: 9.0
      orientation: NORTH
      bounding_box: {
        left: 133.08
        top: 148.26201
        right: 142.08
        bottom: 153.302
      }
      fill_color_hash: 1
    }
  )proto");
  Cluster(&page);
  ASSERT_EQ(page.segments().size(), 1);
  ASSERT_THAT(page.segments(0).character_indices(), ElementsAreArray({0, 1}));
  ASSERT_EQ(page.rows().size(), 1);
  ASSERT_EQ(page.rows(0).blocks().size(), 1);
  EXPECT_EQ(page.rows(0).blocks(0).text(), "Re");
}

TEST(ExtractLine, do_not_connect) {
  PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"proto(
    number: 1
    width: 612
    height: 792
    characters: {
      codepoint: 0x00000049
      utf8: "I"
      font_size: 24.0
      orientation: EAST
      bounding_box: { left: 202.92 top: 165.84 right: 209.328 bottom: 189.84 }
      fill_color_hash: 1
    }
    characters: {
      codepoint: 0x0000006e
      utf8: "n"
      font_size: 24.0
      orientation: EAST
      bounding_box: {
        left: 229.3232  # too far from right of previous char
        top: 165.84
        right: 243.0992
        bottom: 189.84
      }
      fill_color_hash: 1
    }
  )proto");
  Cluster(&page);
  ASSERT_EQ(page.segments().size(), 2);
  ASSERT_THAT(page.segments(0).character_indices(), ElementsAreArray({0}));
  ASSERT_THAT(page.segments(1).character_indices(), ElementsAreArray({1}));
  ASSERT_EQ(page.rows().size(), 1);
  ASSERT_EQ(page.rows(0).blocks().size(), 2);
  EXPECT_EQ(page.rows(0).blocks(0).text(), "I");
  EXPECT_EQ(page.rows(0).blocks(1).text(), "n");
}

}  // namespace

}  // namespace pdf
}  // namespace exegesis
