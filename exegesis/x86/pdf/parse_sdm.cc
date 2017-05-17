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

#include "exegesis/x86/pdf/parse_sdm.h"

#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include "strings/string.h"

#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/x86/pdf/intel_sdm_extractor.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"

namespace exegesis {
namespace x86 {
namespace pdf {
namespace {

using ::exegesis::pdf::LoadConfigurations;
using ::exegesis::pdf::PdfDocument;
using ::exegesis::pdf::PdfDocumentsChanges;
using ::exegesis::pdf::PdfParseRequest;

constexpr const char kSourceName[] = "IntelSDMParser V2";

InstructionSetSourceInfo CreateInstructionSetSourceInfo(
    const google::protobuf::Map<string, string>& map) {
  InstructionSetSourceInfo source_info;
  source_info.set_source_name(kSourceName);

  for (const auto& key_value_pair : map) {
    auto* const metadata = source_info.add_metadata();
    metadata->set_key(key_value_pair.first);
    metadata->set_value(key_value_pair.second);
  }
  return source_info;
}

// Parses the input specification (see FLAGS_exegesis_input_spec for the
// format).
std::vector<PdfParseRequest> ParseRequestsOrDie(const string& input_spec) {
  const std::vector<string> specs =
      strings::Split(input_spec, ",", strings::SkipEmpty());  // NOLINT
  std::vector<PdfParseRequest> parsed_specs;
  parsed_specs.reserve(specs.size());
  for (const string& spec : specs) {
    parsed_specs.push_back(exegesis::pdf::ParseRequestOrDie(spec));
  }
  return parsed_specs;
}

}  // namespace

InstructionSetProto ParseSdmOrDie(const string& input_spec,
                                  const string& patches_folder,
                                  const string& output_base) {
  const PdfDocumentsChanges patch_sets = LoadConfigurations(patches_folder);

  const auto requests = ParseRequestsOrDie(input_spec);

  InstructionSetProto full_instruction_set;
  for (int request_id = 0; request_id < requests.size(); ++request_id) {
    const PdfParseRequest& spec = requests[request_id];
    const PdfDocument pdf_document = ParseOrDie(spec, patch_sets);
    const string pb_filename = StrCat(output_base, "_", request_id, ".pdf.pb");
    LOG(INFO) << "Saving pdf as proto file : " << pb_filename;
    WriteBinaryProtoOrDie(pb_filename, pdf_document);

    LOG(INFO) << "Extracting instruction set";
    const SdmDocument sdm_document =
        ConvertPdfDocumentToSdmDocument(pdf_document);
    const string sdm_pb_filename =
        StrCat(output_base, "_", request_id, ".sdm.pb");
    LOG(INFO) << "Saving pdf as proto file : " << sdm_pb_filename;
    WriteBinaryProtoOrDie(sdm_pb_filename, sdm_document);
    InstructionSetProto instruction_set = ProcessIntelSdmDocument(sdm_document);
    *instruction_set.add_source_infos() =
        CreateInstructionSetSourceInfo(pdf_document.metadata());
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
}  // namespace exegesis
