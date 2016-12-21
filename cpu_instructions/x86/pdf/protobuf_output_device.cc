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

#include "cpu_instructions/x86/pdf/protobuf_output_device.h"

#include <memory>

#include "glog/logging.h"
#include "strings/string_view_utils.h"
#include "libutf/utf.h"
#include "util/gtl/map_util.h"
#include "cpu_instructions/x86/pdf/geometry.h"
#include "cpu_instructions/x86/pdf/pdf_document_parser.h"
#include "cpu_instructions/x86/pdf/pdf_document_utils.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

namespace {

constexpr const int kMinFontSize = 4;

bool StartsWith(const PdfTextSegment& segment, StringPiece expected) {
  return strings::StartsWith(segment.text(), expected);
}

bool StartsWith(const PdfTextSegment& a, StringPiece expected_a,
                const PdfTextSegment& b, StringPiece expected_b) {
  return StartsWith(a, expected_a) && StartsWith(b, expected_b);
}

Orientation GetOrientation(float dx, float dy) {
  if (dx > 0) return Orientation::EAST;
  if (dx < 0) return Orientation::WEST;
  if (dy > 0) return Orientation::SOUTH;
  if (dy < 0) return Orientation::NORTH;
  LOG(FATAL) << "Unreachable code";
  return Orientation::NORTH;  // Never reached.
}

// Returns the BoundingBox for a character at position (x, y) and a particular
// orientation. dx/dy is used in the forward direction (width), font_size is
// used for the height.
BoundingBox GetBoundingBox(const float x, const float y, const float dx,
                           const float dy, const float font_size,
                           const Orientation& orientation) {
  switch (orientation) {
    case Orientation::EAST:
      return CreateBox(x, y - font_size, x + dx, y);
    case Orientation::WEST:
      return CreateBox(x + dx, y - font_size, x, y);
    case Orientation::SOUTH:
      return CreateBox(x, y, x + font_size, y + dy);
    case Orientation::NORTH:
      return CreateBox(x - font_size, y + dy, x, y);
    default:
      return BoundingBox();
  }
}

// Converts the unicode data from xpdf into a string.
string GetUtf8String(Unicode* u, int uLen) {
  CHECK_EQ(uLen, 1);
  char buffer[UTFmax];
  const int length = runetochar(buffer, reinterpret_cast<Rune*>(u));
  const string output(buffer, length);
  // TODO(user): Moves this in the parser configuration.
  if (output == "—") return "-";
  if (output == "–") return "-";
  return output;
}

PdfPageChanges GetPageChanges(const PdfDocumentChanges& document_changes,
                              int page_number) {
  PdfPageChanges result;
  for (const auto& page_changes : document_changes.pages()) {
    if (page_changes.page_number() == page_number) {
      result.MergeFrom(page_changes);
    }
  }
  return result;
}

}  // namespace

ProtobufOutputDevice::ProtobufOutputDevice(
    const PdfDocumentChanges& document_changes, PdfDocument* pdf_document)
    : document_changes_(document_changes), pdf_document_(pdf_document) {}

ProtobufOutputDevice::~ProtobufOutputDevice() {
  LOG(INFO) << "Processing done";
}

void ProtobufOutputDevice::startPage(int pageNum, GfxState* state) {
  current_page_.set_number(pageNum);
  if (state) {
    current_page_.set_width(state->getPageWidth());
    current_page_.set_height(state->getPageHeight());
  }
  LOG_EVERY_N(INFO, 100) << "Processing page " << pageNum;
}

void ProtobufOutputDevice::endPage() {
  const auto page_number = current_page_.number();
  const auto& page_changes = GetPageChanges(document_changes_, page_number);
  Cluster(&current_page_, page_changes.prevent_segment_bindings());
  if (!page_changes.patches().empty()) {
    LOG(INFO) << "Patching page " << page_number;
    for (const auto& patch : page_changes.patches()) {
      ApplyPatchOrDie(patch, &current_page_);
    }
  }
  current_page_.Swap(pdf_document_->add_pages());
}

void ProtobufOutputDevice::drawChar(GfxState* state, double x, double y,
                                    double dx, double dy, double originX,
                                    double originY, CharCode c, int nBytes,
                                    Unicode* u, int uLen) {
  // Subtracts char and word spacing from the dx,dy values.
  double sp = state->getCharSpace();
  if (c == static_cast<CharCode>(0x20)) {
    sp += state->getWordSpace();
  }
  double x1, y1, width, height, dx2, dy2;
  state->textTransformDelta(sp * state->getHorizScaling(), 0, &dx2, &dy2);
  state->transformDelta(dx - dx2, dy - dy2, &width, &height);
  state->transform(x, y, &x1, &y1);

  const float font_size = state->getTransformedFontSize();
  const Orientation orientation = GetOrientation(width, height);

  // Dropping characters smaller than kMinFontSize.
  if (font_size < kMinFontSize) return;

  auto* pdf_char = current_page_.add_characters();
  pdf_char->set_codepoint(c);
  pdf_char->set_utf8(GetUtf8String(u, uLen));
  pdf_char->set_font_size(font_size);
  pdf_char->set_orientation(orientation);
  const char* color_buffer =
      reinterpret_cast<const char*>(CHECK_NOTNULL(state->getFillColor()->c));
  const int color_buffer_size =
      CHECK_NOTNULL(state->getFillColorSpace())->getNComps() *
      sizeof(GfxColorComp);
  pdf_char->set_fill_color_hash(
      std::hash<string>()(string(color_buffer, color_buffer_size)));
  *pdf_char->mutable_bounding_box() =
      GetBoundingBox(x1, y1, width, height, font_size, orientation);
}

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
