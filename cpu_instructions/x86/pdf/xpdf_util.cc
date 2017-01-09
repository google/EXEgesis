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

#include "cpu_instructions/x86/pdf/xpdf_util.h"

#include <functional>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "glog/logging.h"
#include "strings/string_view_utils.h"
#include "libutf/utf.h"
#include "xpdf-3.04/xpdf/GfxState.h"
#include "xpdf-3.04/xpdf/GlobalParams.h"
#include "xpdf-3.04/xpdf/OutputDev.h"
#include "xpdf-3.04/xpdf/PDFDoc.h"
#include "xpdf-3.04/xpdf/PDFDocEncoding.h"
#include "xpdf-3.04/xpdf/UnicodeMap.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"
#include "cpu_instructions/x86/pdf/geometry.h"
#include "cpu_instructions/x86/pdf/pdf_document.pb.h"
#include "cpu_instructions/x86/pdf/pdf_document_parser.h"
#include "cpu_instructions/x86/pdf/pdf_document_utils.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

namespace {

// Display resolution.
constexpr const int kHorizontalDPI = 72;
constexpr const int kVerticalDPI = 72;

constexpr const char kMetadataAuthor[] = "Author";
constexpr const char kMetadataCreationDate[] = "CreationDate";
constexpr const char kMetadataKeywords[] = "Keywords";
constexpr const char kMetadataModificationDate[] = "ModDate";
constexpr const char kMetadataTitle[] = "Title";

constexpr const char* kMetadataEntries[] = {
    kMetadataTitle, kMetadataKeywords, kMetadataAuthor, kMetadataCreationDate,
    kMetadataModificationDate};

// Returns the singleton xpdf global parameters.
GlobalParams* GetXpdfGlobalParams() {
  // Initialize once.
  static GlobalParams* const result = []() {
    // xpdf reads options from a global variable :(
    globalParams = new GlobalParams(nullptr);
    globalParams->setTextEncoding(const_cast<char*>("UTF-8"));
    return globalParams;
  }();
  return result;
}

// Reads PDF metadata.
XPDFDoc::Metadata ReadMetadata(PDFDoc* doc) {
  XPDFDoc::Metadata metadata_map;
  UnicodeMap* const unicode_map = GetXpdfGlobalParams()->getTextEncoding();

  Object info;
  doc->getDocInfo(&info);
  if (!info.isDict()) {
    LOG(WARNING) << "PDF has no metadata entries";
    info.free();
    return metadata_map;
  }
  for (const char* key : kMetadataEntries) {
    Object obj;
    if (info.getDict()->lookup(key, &obj)->isString()) {
      auto* value = obj.getString();
      // The metadata can be pdf encoding (default) or ucs-2 (if a Byte Order
      // Mark is present, see
      // https://en.wikipedia.org/wiki/Byte_order_mark#UTF-16).
      const bool is_ucs2 = (value->getChar(0) & 0xff) == 0xfe &&
                           (value->getChar(1) & 0xff) == 0xff;
      char utf8_buffer[2];
      for (int i = is_ucs2 ? 2 : 0; i < value->getLength();) {
        int unicode = 0;
        if (is_ucs2) {
          unicode = ((value->getChar(i) & 0xff) << 8) |
                    (value->getChar(i + 1) & 0xff);
          i += 2;
        } else {
          unicode = pdfDocEncoding[value->getChar(i) & 0xff];
          ++i;
        }
        const int num_utf8_bytes =
            unicode_map->mapUnicode(unicode, utf8_buffer, sizeof(utf8_buffer));
        metadata_map[key].append(utf8_buffer, num_utf8_bytes);
      }
    }
    obj.free();
  }
  info.free();
  return metadata_map;
}

PdfDocumentId CreateDocumentId(const XPDFDoc::Metadata& map) {
  PdfDocumentId document_id;
  if (ContainsKey(map, kMetadataTitle))
    document_id.set_title(FindOrDie(map, kMetadataTitle));
  if (ContainsKey(map, kMetadataCreationDate))
    document_id.set_creation_date(FindOrDie(map, kMetadataCreationDate));
  if (ContainsKey(map, kMetadataModificationDate))
    document_id.set_modification_date(
        FindOrDie(map, kMetadataModificationDate));
  return document_id;
}

}  // namespace

std::unique_ptr<const XPDFDoc> XPDFDoc::OpenOrDie(const string& filename) {
  GetXpdfGlobalParams();  // Maybe initialize xpdf globals.
  auto doc =
      gtl::MakeUnique<PDFDoc>(new GString(filename.c_str()), nullptr, nullptr);
  CHECK(doc->isOk()) << "Could not open PDF file: '" << filename << "'";
  CHECK_GT(doc->getNumPages(), 0);
  return std::unique_ptr<const XPDFDoc>(new XPDFDoc(std::move(doc)));
}

XPDFDoc::XPDFDoc(std::unique_ptr<PDFDoc> doc)
    : doc_(std::move(doc)),
      metadata_(ReadMetadata(doc_.get())),
      doc_id_(CreateDocumentId(metadata_)) {}

XPDFDoc::~XPDFDoc() {}

namespace {

// An XPDF device which outputs the stream of characters as a PdfDocument
// protobuf.
class ProtobufOutputDevice : public OutputDev {
 public:
  // PdfDocumentChanges is used to change the way the document is parsed, it is
  // also responsible for patching the document afterwards.
  // ProtobufOutputDevice does not acquire ownership of pdf_document.
  // pdf_document should outlive this instance.
  ProtobufOutputDevice(const PdfDocumentChanges& document_changes,
                       PdfDocument* pdf_document)
      : document_changes_(document_changes), pdf_document_(pdf_document) {}

  ProtobufOutputDevice(const ProtobufOutputDevice&) = delete;

  ~ProtobufOutputDevice() override { LOG(INFO) << "Processing done"; }

 private:
  GBool upsideDown() override { return gTrue; }
  GBool useDrawChar() override { return gTrue; }
  GBool interpretType3Chars() override { return gFalse; }
  GBool needNonText() override { return gFalse; }

  void startPage(int pageNum, GfxState* state) override;
  void endPage() override;
  void drawChar(GfxState* state, double x, double y, double dx, double dy,
                double originX, double originY, CharCode c, int nBytes,
                Unicode* u, int uLen) override;

  const PdfDocumentChanges document_changes_;
  PdfDocument* const pdf_document_ = nullptr;
  PdfPage current_page_;
};

constexpr const int kMinFontSize = 4;

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

}  // namespace

PdfDocument XPDFDoc::Parse(const int first_page, const int last_page,
                           const PdfDocumentChanges& patches) const {
  PdfDocument pdf_document;
  ProtobufOutputDevice output_device(patches, &pdf_document);
  doc_->displayPages(&output_device, first_page,
                     last_page <= 0 ? doc_->getNumPages() : last_page,
                     kHorizontalDPI, kVerticalDPI, /* rotate= */ 0,
                     /* useMediaBox= */ gTrue, /* crop= */ gTrue,
                     /* printing= */ gTrue);
  return pdf_document;
}

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
