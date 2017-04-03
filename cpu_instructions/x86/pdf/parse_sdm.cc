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

#include "cpu_instructions/base/pdf/pdf_document_utils.h"
#include "cpu_instructions/base/pdf/xpdf_util.h"
#include "cpu_instructions/util/proto_util.h"
#include "cpu_instructions/x86/pdf/intel_sdm_extractor.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {
namespace {

constexpr const char kSourceName[] = "IntelSDMParser V2";

InstructionSetSourceInfo CreateInstructionSetSourceInfo(
    const XPDFDoc::Metadata& map) {
  InstructionSetSourceInfo source_info;
  source_info.set_source_name(kSourceName);

  for (const auto& key_value_pair : map) {
    auto* const metadata = source_info.add_metadata();
    metadata->set_key(key_value_pair.first);
    metadata->set_value(key_value_pair.second);
  }
  return source_info;
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

  InstructionSetProto full_instruction_set;

  for (int spec_id = 0; spec_id < input_specs.size(); ++spec_id) {
    const InputSpec& input_spec = input_specs[spec_id];
    // Open document. PDFDoc takes ownership of the name.
    LOG(INFO) << "Opening PDF file : " << input_spec.filename;
    const auto doc = XPDFDoc::OpenOrDie(input_spec.filename);
    const auto& pdf_document_id = doc->GetDocumentId();
    const auto* config = GetConfigOrNull(patch_sets, pdf_document_id);
    CHECK(config) << "Unsupported version. Metadata:\n"
                  << pdf_document_id.DebugString();

    LOG(INFO) << "Reading PDF file";
    const PdfDocument pdf_document =
        doc->Parse(input_spec.first_page, input_spec.last_page, *config);
    const string pb_filename = StrCat(output_base, "_", spec_id, ".pdf.pb");
    LOG(INFO) << "Saving pdf as proto file : " << pb_filename;
    WriteBinaryProtoOrDie(pb_filename, pdf_document);

    LOG(INFO) << "Extracting instruction set";
    const SdmDocument sdm_document =
        ConvertPdfDocumentToSdmDocument(pdf_document);
    const string sdm_pb_filename = StrCat(output_base, "_", spec_id, ".sdm.pb");
    LOG(INFO) << "Saving pdf as proto file : " << sdm_pb_filename;
    WriteBinaryProtoOrDie(sdm_pb_filename, sdm_document);
    InstructionSetProto instruction_set = ProcessIntelSdmDocument(sdm_document);
    *instruction_set.add_source_infos() =
        CreateInstructionSetSourceInfo(doc->GetMetadata());
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
