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

// This program helps creating patches for a protobuf version of a pdf document
// created with the pdf2proto tool.
// Usage:
// bazel run -c opt \
// exegesis/tools:proto_patch_helper -- \
// --exegesis_proto_input_file=/path/to/sdm.pdf.pb \
// --exegesis_match_expression='SAL/SAR/SHL/SHR' \
// --exegesis_page_numbers=662

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gflags/gflags.h"

#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "util/gtl/map_util.h"

DEFINE_string(exegesis_from_proto_file, "", "");
DEFINE_string(exegesis_to_proto_file, "", "");
DEFINE_string(exegesis_output_file_base, "", "");
DEFINE_string(
    exegesis_patches_directory, "exegesis/x86/pdf/sdm_patches/",
    "A folder containing a set of patches to apply to original documents");

namespace exegesis {
namespace pdf {
namespace {

std::string GetFilename(const std::string& name) {
  return StrCat(FLAGS_exegesis_output_file_base, "_", name, ".pb.txt");
}

void WritePatchesOrDie(const std::string& name,
                       const PdfDocumentChanges& changes) {
  const std::string filename = GetFilename(name);
  WriteTextProtoOrDie(filename, changes);
  size_t count = 0;
  for (const auto& page : changes.pages()) {
    count += page.patches_size();
  }
  LOG(INFO) << "Wrote " << filename << " with " << count << " patch"
            << (count >= 2 ? "es" : "");
}

const PdfDocumentChanges& FindPatchesOrDie(
    const PdfDocument& document, const PdfDocumentsChanges& patch_sets) {
  const auto* changes = GetConfigOrNull(patch_sets, document.document_id());
  CHECK(changes) << "Can't find patches for document \n"
                 << document.document_id().DebugString();
  return *changes;
}

void CheckPatchesOrDie(const PdfDocument& document,
                       const PdfDocumentChanges& changes) {
  const auto FindPage = [&document](size_t page_number) {
    for (const auto& page : document.pages()) {
      if (page.number() == page_number) {
        return page;
      }
    }
    LOG(FATAL) << "Can't find page " << page_number << " in original document";
  };
  for (const auto& page_changes : changes.pages()) {
    const auto page_number = page_changes.page_number();
    for (const auto& patch : page_changes.patches()) {
      const auto& page = FindPage(page_number);
      const std::string found =
          GetCellTextOrEmpty(page, patch.row(), patch.col());
      CHECK_EQ(patch.expected(), found)
          << "The original patch is invalid at page " << page.number()
          << ", row " << patch.row() << ", col " << patch.col();
    }
  }
}

void Main() {
  CHECK(!FLAGS_exegesis_from_proto_file.empty())
      << "missing --exegesis_from_proto_file";
  CHECK(!FLAGS_exegesis_to_proto_file.empty())
      << "missing --exegesis_to_proto_file";
  CHECK(!FLAGS_exegesis_patches_directory.empty())
      << "missing --exegesis_patches_directory";
  CHECK(!FLAGS_exegesis_output_file_base.empty())
      << "missing --exegesis_output_file_base";

  LOG(INFO) << "Opening original document " << FLAGS_exegesis_from_proto_file;
  const auto from_document =
      ReadBinaryProtoOrDie<PdfDocument>(FLAGS_exegesis_from_proto_file);
  LOG(INFO) << "Opening patches from " << FLAGS_exegesis_patches_directory;
  const auto patch_sets = LoadConfigurations(FLAGS_exegesis_patches_directory);
  LOG(INFO) << "Finding original patches";
  const auto& changes = FindPatchesOrDie(from_document, patch_sets);
  LOG(INFO) << "Checking patches";
  CheckPatchesOrDie(from_document, changes);
  LOG(INFO) << "Opening destination document " << FLAGS_exegesis_to_proto_file;
  const auto to_document =
      ReadBinaryProtoOrDie<PdfDocument>(FLAGS_exegesis_to_proto_file);

  PdfDocumentChanges successful_patches;
  PdfDocumentChanges failed_patches;
  TransferPatches(changes, from_document, to_document, &successful_patches,
                  &failed_patches);

  WritePatchesOrDie("failed_patches", failed_patches);
  WritePatchesOrDie("successful_patches", successful_patches);
}

}  // namespace
}  // namespace pdf
}  // namespace exegesis

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  ::exegesis::pdf::Main();
  return 0;
}
