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

#include "cpu_instructions/base/pdf/xpdf_util.h"

#include "cpu_instructions/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "strings/str_cat.h"
#include "util/gtl/ptr_util.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {
namespace {

using ::cpu_instructions::testing::EqualsProto;

const char kTestDataPath[] = "/__main__/cpu_instructions/base/pdf/testdata/";
constexpr const int kHorizontalDPI = 72;
constexpr const int kVerticalDPI = 72;

string GetPdfFilename(const string& name) {
  return StrCat(getenv("TEST_SRCDIR"), kTestDataPath, name);
}

TEST(ProtobufOutputDeviceTest, TestSimplePdfOutput) {
  const auto doc = XPDFDoc::OpenOrDie(GetPdfFilename("simple.pdf"));

  EXPECT_THAT(doc->GetDocumentId(), EqualsProto(""));

  PdfDocument pdf_document =
      doc->Parse(1 /*first_page*/, -1 /*last_page*/, PdfDocumentChanges());
  pdf_document.PrintDebugString();
  // We don't care about the hash; it can vary depending on the compiler.
  for (PdfPage& page : *pdf_document.mutable_pages()) {
    for (PdfCharacter& character : *page.mutable_characters()) {
      character.clear_fill_color_hash();
    }
    for (PdfTextSegment& segment : *page.mutable_segments()) {
      segment.clear_fill_color_hash();
    }
  }

  constexpr char kExpected[] = R"(
    pages {
      number: 1
      width: 612
      height: 792
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 72
          top: 72.25
          right: 75.05615
          bottom: 83.25
        }
      }
      characters {
        codepoint: 68
        utf8: "a"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 78
          top: 93.25
          right: 84.117676
          bottom: 104.25
        }
      }
      characters {
        codepoint: 69
        utf8: "b"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 84.117676
          top: 93.25
          right: 90.23535
          bottom: 104.25
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 90
          top: 93.25
          right: 93.05615
          bottom: 104.25
        }
      }
      characters {
        codepoint: 70
        utf8: "c"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 312
          top: 93.25
          right: 317.5
          bottom: 104.25
        }
      }
      characters {
        codepoint: 71
        utf8: "d"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 317.5
          top: 93.25
          right: 323.61768
          bottom: 104.25
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 323.25
          top: 93.25
          right: 326.30615
          bottom: 104.25
        }
      }
      characters {
        codepoint: 72
        utf8: "e"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 312
          top: 106.75
          right: 318.11768
          bottom: 117.75
        }
      }
      characters {
        codepoint: 73
        utf8: "f"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 318.11334
          top: 106.75
          right: 321.1695
          bottom: 117.75
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 321
          top: 106.75
          right: 324.05615
          bottom: 117.75
        }
      }
      characters {
        codepoint: 74
        utf8: "g"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 78
          top: 131.5
          right: 84.117676
          bottom: 142.5
        }
      }
      characters {
        codepoint: 75
        utf8: "h"
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 84.117676
          top: 131.5
          right: 90.23535
          bottom: 142.5
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 90
          top: 131.5
          right: 93.05615
          bottom: 142.5
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 312
          top: 131.5
          right: 315.05615
          bottom: 142.5
        }
      }
      characters {
        codepoint: 3
        utf8: " "
        font_size: 11
        orientation: EAST
        bounding_box {
          left: 72
          top: 151
          right: 75.05615
          bottom: 162
        }
      }
      segments {
        bounding_box {
          left: 72
          top: 72.25
          right: 75.05615
          bottom: 83.25
        }
        orientation: SOUTH
        font_size: 11
        text: " "
        character_indices: 0
      }
      segments {
        bounding_box {
          left: 78
          top: 93.25
          right: 93.05615
          bottom: 104.25
        }
        orientation: SOUTH
        font_size: 11
        text: "ab "
        character_indices: 1
        character_indices: 2
        character_indices: 3
      }
      segments {
        bounding_box {
          left: 312
          top: 93.25
          right: 326.30615
          bottom: 104.25
        }
        orientation: SOUTH
        font_size: 11
        text: "cd "
        character_indices: 4
        character_indices: 5
        character_indices: 6
      }
      segments {
        bounding_box {
          left: 312
          top: 106.75
          right: 324.05615
          bottom: 117.75
        }
        orientation: SOUTH
        font_size: 11
        text: "ef "
        character_indices: 7
        character_indices: 8
        character_indices: 9
      }
      segments {
        bounding_box {
          left: 78
          top: 131.5
          right: 93.05615
          bottom: 142.5
        }
        orientation: SOUTH
        font_size: 11
        text: "gh "
        character_indices: 10
        character_indices: 11
        character_indices: 12
      }
      segments {
        bounding_box {
          left: 312
          top: 131.5
          right: 315.05615
          bottom: 142.5
        }
        orientation: SOUTH
        font_size: 11
        text: " "
        character_indices: 13
      }
      segments {
        bounding_box {
          left: 72
          top: 151
          right: 75.05615
          bottom: 162
        }
        orientation: SOUTH
        font_size: 11
        text: " "
        character_indices: 14
      }
      blocks {
        bounding_box {
          left: 72
          top: 72.25
          right: 75.05615
          bottom: 83.25
        }
        orientation: SOUTH
        font_size: 11
        text: " "
      }
      blocks {
        bounding_box {
          left: 78
          top: 93.25
          right: 93.05615
          bottom: 104.25
        }
        orientation: SOUTH
        font_size: 11
        text: "ab "
      }
      blocks {
        bounding_box {
          left: 312
          top: 93.25
          right: 326.30615
          bottom: 117.75
        }
        orientation: SOUTH
        font_size: 11
        text: "cd \nef "
      }
      blocks {
        bounding_box {
          left: 78
          top: 131.5
          right: 93.05615
          bottom: 142.5
        }
        orientation: SOUTH
        font_size: 11
        text: "gh "
      }
      blocks {
        bounding_box {
          left: 312
          top: 131.5
          right: 315.05615
          bottom: 142.5
        }
        orientation: SOUTH
        font_size: 11
        text: " "
      }
      blocks {
        bounding_box {
          left: 72
          top: 151
          right: 75.05615
          bottom: 162
        }
        orientation: SOUTH
        font_size: 11
        text: " "
      }
      rows {
        blocks {
          bounding_box {
            left: 72
            top: 72.25
            right: 75.05615
            bottom: 83.25
          }
          font_size: 11
        }
        bounding_box {
          left: 72
          top: 72.25
          right: 75.05615
          bottom: 83.25
        }
      }
      rows {
        blocks {
          bounding_box {
            left: 78
            top: 93.25
            right: 93.05615
            bottom: 104.25
          }
          font_size: 11
          text: "ab"
        }
        blocks {
          bounding_box {
            left: 312
            top: 93.25
            right: 326.30615
            bottom: 117.75
          }
          font_size: 11
          text: "cd \nef"
        }
        bounding_box {
          left: 78
          top: 93.25
          right: 326.30615
          bottom: 117.75
        }
      }
      rows {
        blocks {
          bounding_box {
            left: 78
            top: 131.5
            right: 93.05615
            bottom: 142.5
          }
          font_size: 11
          text: "gh"
        }
        blocks {
          bounding_box {
            left: 312
            top: 131.5
            right: 315.05615
            bottom: 142.5
          }
          font_size: 11
        }
        bounding_box {
          left: 78
          top: 131.5
          right: 315.05615
          bottom: 142.5
        }
      }
      rows {
        blocks {
          bounding_box {
            left: 72
            top: 151
            right: 75.05615
            bottom: 162
          }
          font_size: 11
        }
        bounding_box {
          left: 72
          top: 151
          right: 75.05615
          bottom: 162
        }
      }
    }
  )";
  EXPECT_THAT(pdf_document, EqualsProto(kExpected));
}

}  // namespace
}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
