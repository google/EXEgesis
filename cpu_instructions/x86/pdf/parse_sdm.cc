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

#include "cpu_instructions/x86/pdf/parse_sdm.h"

#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include "strings/string.h"

#include "glog/logging.h"
#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "xpdf-3.04/xpdf/GlobalParams.h"
#include "xpdf-3.04/xpdf/PDFDoc.h"
#include "xpdf-3.04/xpdf/PDFDocEncoding.h"
#include "xpdf-3.04/xpdf/UnicodeMap.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"
#include "cpu_instructions/proto/cpu_info.pb.h"
#include "cpu_instructions/x86/pdf/intel_sdm_extractor.h"
#include "cpu_instructions/x86/pdf/pdf_document_utils.h"
#include "cpu_instructions/x86/pdf/proto_util.h"
#include "cpu_instructions/x86/pdf/protobuf_output_device.h"
#include "re2/re2.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {
namespace {

// Display resolution.
constexpr const int kHorizontalDPI = 72;
constexpr const int kVerticalDPI = 72;

constexpr const char kSourceName[] = "IntelSDMParser V2";

constexpr const char kMetadataAuthor[] = "Author";
constexpr const char kMetadataCreationDate[] = "CreationDate";
constexpr const char kMetadataKeywords[] = "Keywords";
constexpr const char kMetadataModificationDate[] = "ModDate";
constexpr const char kMetadataTitle[] = "Title";

constexpr const char* kMetadataEntries[] = {
    kMetadataTitle, kMetadataKeywords, kMetadataAuthor, kMetadataCreationDate,
    kMetadataModificationDate};

typedef std::map<string, string> Metadata;

// Reads PDF metadata.
Metadata ReadMetadata(PDFDoc* doc, UnicodeMap* unicode_map) {
  Metadata metadata_map;

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

InstructionSetSourceInfo CreateInstructionSetSourceInfo(const Metadata& map) {
  InstructionSetSourceInfo source_info;
  source_info.set_source_name(kSourceName);

  for (const auto& key_value_pair : map) {
    auto* const metadata = source_info.add_metadata();
    metadata->set_key(key_value_pair.first);
    metadata->set_value(key_value_pair.second);
  }
  return source_info;
}

PdfDocumentId CreateDocumentId(const Metadata& map) {
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

// Represents a single input file and page range.
struct InputSpec {
  explicit InputSpec(const string& spec) {
    CHECK(RE2::FullMatch(spec, R"(([^:]+)(:[0-9]+-[0-9]+)?)", &filename))
        << "Invalid spec '" << spec << "'";
    RE2::FullMatch(spec, R"([^:]+:([0-9]+)-([0-9]+))", &first_page, &last_page);
  }

  string filename;
  int first_page = 1;
  int last_page = 0;
};

// Parses the input specification (see FLAGS_cpu_instructions_input_spec for the
// format).
std::vector<InputSpec> ParseInputSpec(const string& input_spec) {
  const std::vector<string> specs =
      strings::Split(input_spec, ",", strings::SkipEmpty());  // NOLINT
  std::vector<InputSpec> parsed_specs;
  for (const string& spec : specs) {
    parsed_specs.emplace_back(spec);
  }
  return parsed_specs;
}

const PdfDocumentChanges* GetConfigOrNull(const PdfDocumentsChanges& patch_sets,
                                          const PdfDocumentId& document_id) {
  for (const auto& document : patch_sets.documents()) {
    const auto& current_id = document.document_id();
    if (current_id.title() == document_id.title() &&
        current_id.creation_date() == document_id.creation_date() &&
        current_id.modification_date() == document_id.modification_date()) {
      return &document;
    }
  }
  return nullptr;
}

}  // namespace

InstructionSetProto ParseSdmOrDie(const string& input_spec,
                                  const string& patch_sets_file,
                                  const string& output_base) {
  // Read the input files
  PdfDocumentsChanges patch_sets;
  if (!patch_sets_file.empty()) {
    ReadTextProtoOrDie(patch_sets_file, &patch_sets);
  }

  const auto input_specs = ParseInputSpec(input_spec);

  // xpdf reads options from a global variable :(
  const auto global_params = gtl::MakeUnique<GlobalParams>(nullptr);
  global_params->setTextEncoding(const_cast<char*>("UTF-8"));
  globalParams = global_params.get();

  InstructionSetProto full_instruction_set;
  full_instruction_set.mutable_cpu_info()->set_architecture("x86_64");

  for (int spec_id = 0; spec_id < input_specs.size(); ++spec_id) {
    const InputSpec& input_spec = input_specs[spec_id];
    // Open document. PDFDoc takes ownership of the name.
    LOG(INFO) << "Opening PDF file : " << input_spec.filename;
    PDFDoc doc(new GString(input_spec.filename.c_str()), nullptr, nullptr);
    CHECK(doc.isOk()) << "Could not open PDF file: '" << input_spec.filename
                      << "'";
    CHECK_GT(doc.getNumPages(), 0);
    PdfDocument pdf_document;
    LOG(INFO) << "Reading metadata and check version";
    const Metadata metadata =
        ReadMetadata(&doc, globalParams->getTextEncoding());
    const auto pdf_document_id = CreateDocumentId(metadata);
    const auto* config = GetConfigOrNull(patch_sets, pdf_document_id);
    CHECK(config) << "Unsupported version. Metadata:\n"
                  << pdf_document_id.DebugString();
    InstructionSetProto instruction_set;
    *instruction_set.add_source_info() =
        CreateInstructionSetSourceInfo(metadata);

    LOG(INFO) << "Reading PDF file";
    ProtobufOutputDevice output_device(*config, &pdf_document);
    doc.displayPages(
        &output_device, input_spec.first_page,
        input_spec.last_page <= 0 ? doc.getNumPages() : input_spec.last_page,
        kHorizontalDPI, kVerticalDPI, /* rotate= */ 0,
        /* useMediaBox= */ gTrue, /* crop= */ gTrue,
        /* printing= */ gTrue);
    const string pb_filename = StrCat(output_base, "_", spec_id, ".pdf.pb");
    LOG(INFO) << "Saving pdf as proto file : " << pb_filename;
    WriteBinaryProtoOrDie(pb_filename, pdf_document);

    LOG(INFO) << "Extracting instruction set";
    SdmDocument sdm_document;
    ProcessIntelPdfDocument(&pdf_document, &sdm_document);
    const string sdm_pb_filename = StrCat(output_base, "_", spec_id, ".sdm.pb");
    LOG(INFO) << "Saving pdf as proto file : " << sdm_pb_filename;
    WriteBinaryProtoOrDie(sdm_pb_filename, sdm_document);
    ProcessIntelSdmDocument(&sdm_document, &instruction_set);
    full_instruction_set.MergeFrom(instruction_set);
  }

  // Outputs the instructions.
  const string instructions_filename = StrCat(output_base, ".pbtxt");
  LOG(INFO) << "Saving instruction database as: " << instructions_filename;
  WriteTextProtoOrDie(instructions_filename, full_instruction_set);

  return full_instruction_set;
}

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
