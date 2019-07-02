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
#include <string>

#include "absl/flags/flag.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/x86/pdf/intel_sdm_extractor.h"
#include "exegesis/x86/registers.h"
#include "glog/logging.h"
#include "net/proto2/util/public/repeated_field_util.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"

ABSL_FLAG(bool, exegesis_parse_sdm_store_intermediate_files, false,
          "Set to true to write intermediate files: the PDF and SDM protos "
          "and the raw instruction set.");

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
    const google::protobuf::Map<std::string, std::string>& map) {
  InstructionSetSourceInfo source_info;
  source_info.set_source_name(kSourceName);

  for (const auto& key_value_pair : map) {
    auto* const metadata = source_info.add_metadata();
    metadata->set_key(key_value_pair.first);
    metadata->set_value(key_value_pair.second);
  }

  // Making sure metadata always appear in the same order.
  Sort(source_info.mutable_metadata(),
       [](const InstructionSetSourceInfo::MetadataEntry* const a,
          const InstructionSetSourceInfo::MetadataEntry* const b) {
         return a->key() < b->key();
       });

  return source_info;
}

// Parses the input specification (see FLAGS_exegesis_input_spec for the
// format).
std::vector<PdfParseRequest> ParseRequestsOrDie(const std::string& input_spec) {
  const std::vector<std::string> specs =
      absl::StrSplit(input_spec, ",", absl::SkipEmpty());  // NOLINT
  std::vector<PdfParseRequest> parsed_specs;
  parsed_specs.reserve(specs.size());
  for (const std::string& spec : specs) {
    parsed_specs.push_back(exegesis::pdf::ParseRequestOrDie(spec));
  }
  return parsed_specs;
}

}  // namespace

ArchitectureProto ParseSdmOrDie(const std::string& input_spec,
                                const std::string& patches_folder,
                                const std::string& output_base) {
  const PdfDocumentsChanges patch_sets = LoadConfigurations(patches_folder);

  const auto requests = ParseRequestsOrDie(input_spec);

  ArchitectureProto architecture;
  InstructionSetProto* const full_instruction_set =
      architecture.mutable_instruction_set();
  for (int request_id = 0; request_id < requests.size(); ++request_id) {
    const PdfParseRequest& spec = requests[request_id];
    const PdfDocument pdf_document = ParseOrDie(spec, patch_sets);
    if (absl::GetFlag(FLAGS_exegesis_parse_sdm_store_intermediate_files)) {
      const std::string pb_filename =
          absl::StrCat(output_base, "_", request_id, ".pdf.pb");
      LOG(INFO) << "Saving pdf as proto file : " << pb_filename;
      WriteBinaryProtoOrDie(pb_filename, pdf_document);
    }

    LOG(INFO) << "Extracting instruction set";
    const SdmDocument sdm_document =
        ConvertPdfDocumentToSdmDocument(pdf_document);
    if (absl::GetFlag(FLAGS_exegesis_parse_sdm_store_intermediate_files)) {
      const std::string sdm_pb_filename =
          absl::StrCat(output_base, "_", request_id, ".sdm.pb");
      LOG(INFO) << "Saving pdf as proto file : " << sdm_pb_filename;
      WriteBinaryProtoOrDie(sdm_pb_filename, sdm_document);
    }
    InstructionSetProto instruction_set = ProcessIntelSdmDocument(sdm_document);
    *instruction_set.add_source_infos() =
        CreateInstructionSetSourceInfo(pdf_document.metadata());
    full_instruction_set->MergeFrom(instruction_set);
  }

  // Add information about registers; the registers are not listed in the SDM in
  // a consistent way, and thus we supply our own definitions.
  *architecture.mutable_register_set() = x86::GetRegisterSet();

  // Outputs the instructions.
  if (absl::GetFlag(FLAGS_exegesis_parse_sdm_store_intermediate_files)) {
    const std::string instructions_filename =
        absl::StrCat(output_base, ".raw.pbtxt");
    LOG(INFO) << "Saving instruction database as: " << instructions_filename;
    WriteTextProtoOrDie(instructions_filename, architecture);
  }

  return architecture;
}

}  // namespace pdf
}  // namespace x86
}  // namespace exegesis
