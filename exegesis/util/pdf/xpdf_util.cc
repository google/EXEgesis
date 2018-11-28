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

#include "exegesis/util/pdf/xpdf_util.h"

#include <functional>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "absl/memory/memory.h"
#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/geometry.h"
#include "exegesis/util/pdf/pdf_document_parser.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "glog/logging.h"
#include "libutf/utf.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"
#include "xpdf-3.04/xpdf/GfxState.h"
#include "xpdf-3.04/xpdf/GlobalParams.h"
#include "xpdf-3.04/xpdf/OutputDev.h"
#include "xpdf-3.04/xpdf/PDFDoc.h"
#include "xpdf-3.04/xpdf/PDFDocEncoding.h"
#include "xpdf-3.04/xpdf/UnicodeMap.h"

namespace exegesis {
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
void ReadMetadata(PDFDoc* doc, PdfDocument* document) {
  CHECK(document != nullptr);
  CHECK(doc != nullptr);
  google::protobuf::Map<std::string, std::string>& metadata_map =
      *document->mutable_metadata();
  UnicodeMap* const unicode_map = GetXpdfGlobalParams()->getTextEncoding();

  Object info;
  doc->getDocInfo(&info);
  if (!info.isDict()) {
    LOG(WARNING) << "PDF has no metadata entries";
    info.free();
    return;
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
}

void CreateDocumentId(PdfDocument* document) {
  CHECK(document != nullptr);

  const google::protobuf::Map<std::string, std::string>& map =
      document->metadata();
  PdfDocumentId* const document_id = document->mutable_document_id();
  if (gtl::ContainsKey(map, kMetadataTitle))
    document_id->set_title(gtl::FindOrDie(map, kMetadataTitle));
  if (gtl::ContainsKey(map, kMetadataCreationDate))
    document_id->set_creation_date(gtl::FindOrDie(map, kMetadataCreationDate));
  if (gtl::ContainsKey(map, kMetadataModificationDate))
    document_id->set_modification_date(
        gtl::FindOrDie(map, kMetadataModificationDate));
}

std::unique_ptr<PDFDoc> OpenOrDie(const std::string& filename) {
  GetXpdfGlobalParams();  // Maybe initialize xpdf globals.
  auto doc = absl::make_unique<PDFDoc>(new GString(filename.c_str()), nullptr,
                                       nullptr);
  CHECK(doc->isOk()) << "Could not open PDF file: '" << filename << "'";
  CHECK_GT(doc->getNumPages(), 0) << "PDF has no pages: '" << filename << "'";
  return doc;
}

}  // namespace

namespace {

// An XPDF device which outputs the stream of characters as a PdfDocument
// protobuf.
class ProtobufOutputDevice : public OutputDev {
 public:
  // PdfDocumentChanges is used to change the way the document is parsed, it is
  // also responsible for patching the document afterwards.
  // ProtobufOutputDevice does not acquire ownership of pdf_document.
  // pdf_document should outlive this instance.
  ProtobufOutputDevice(const BoundingBox* restrict_to,
                       const PdfDocumentChanges& document_changes,
                       PdfDocument* pdf_document)
      : restrict_to_(restrict_to),
        document_changes_(&document_changes),
        pdf_document_(pdf_document) {}

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

  const BoundingBox* const restrict_to_ = nullptr;
  const PdfDocumentChanges* const document_changes_;
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
std::string GetUtf8String(Unicode* u, int uLen) {
  CHECK_EQ(uLen, 1);
  char buffer[UTFmax];
  const int length = runetochar(buffer, reinterpret_cast<Rune*>(u));
  const std::string output(buffer, length);
  // TODO(gchatelet): Moves this in the parser configuration.
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
  const auto& page_changes = GetPageChanges(*document_changes_, page_number);
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
  const BoundingBox bounding_box =
      GetBoundingBox(x1, y1, width, height, font_size, orientation);

  // Dropping empty characters.
  if (uLen == 0) return;

  // Dropping characters smaller than kMinFontSize.
  if (font_size < kMinFontSize) return;

  // Dropping characters that are outside of the restrict_to area.
  if (restrict_to_ && !Contains(*restrict_to_, bounding_box)) return;

  auto* pdf_char = current_page_.add_characters();
  pdf_char->set_codepoint(c);
  pdf_char->set_utf8(GetUtf8String(u, uLen));
  pdf_char->set_font_size(font_size);
  pdf_char->set_orientation(orientation);
  const char* color_buffer =
      reinterpret_cast<const char*>(state->getFillColor()->c);
  CHECK(state->getFillColorSpace() != nullptr);
  const int color_buffer_size =
      state->getFillColorSpace()->getNComps() * sizeof(GfxColorComp);
  pdf_char->set_fill_color_hash(
      std::hash<std::string>()(std::string(color_buffer, color_buffer_size)));
  *pdf_char->mutable_bounding_box() = bounding_box;
}

}  // namespace

PdfParseRequest ParseRequestOrDie(const std::string& spec) {
  PdfParseRequest request;
  CHECK(RE2::FullMatch(spec, R"(([^:]+)(:[0-9]+-[0-9]+)?)",
                       request.mutable_filename()))
      << "Invalid spec '" << spec << "'";
  int first_page = 0;
  int last_page = 0;
  RE2::FullMatch(spec, R"([^:]+:([0-9]+)-([0-9]+))", &first_page, &last_page);
  request.set_first_page(first_page);
  request.set_last_page(last_page);
  return request;
}

PdfDocument ParseOrDie(const PdfParseRequest& request,
                       const PdfDocumentsChanges& all_patches) {
  const std::unique_ptr<PDFDoc> pdf_doc = OpenOrDie(request.filename());
  PdfDocument document;
  ReadMetadata(pdf_doc.get(), &document);
  CreateDocumentId(&document);
  const auto* const patches =
      GetConfigOrNull(all_patches, document.document_id());
  CHECK(all_patches.documents().empty() || patches != nullptr)
      << "Unable to find document_id '" << document.document_id().DebugString()
      << "' in '" << request.filename() << "'";
  const PdfDocumentChanges no_patch;
  const auto& restrict_to = request.restrict_to();
  const bool is_restricted = restrict_to.right() || restrict_to.bottom();
  ProtobufOutputDevice output_device(is_restricted ? &restrict_to : nullptr,
                                     patches ? *patches : no_patch, &document);
  const int num_pages = pdf_doc->getNumPages();
  const int first_page = request.first_page() == 0 ? 1 : request.first_page();
  const int last_page =
      request.last_page() == 0 ? num_pages : request.last_page();
  pdf_doc->displayPages(&output_device,                //
                        first_page, last_page,         //
                        kHorizontalDPI, kVerticalDPI,  //
                        /* rotate= */ 0,
                        /* useMediaBox= */ gTrue, /* crop= */ gTrue,
                        /* printing= */ gTrue);
  return document;
}
}  // namespace pdf
}  // namespace exegesis
