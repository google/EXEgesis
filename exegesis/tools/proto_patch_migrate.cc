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

// This program transfers patches from one version of a PDF document to another.
// The tool creates two files containing a list of patches: one for patches that
// were successfully applied to the new file and one for patches that could not
// be applied.
//
// Usage:
//  bazel run -c opt \
//    exegesis/tools:proto_patch_migrate -- \
//    exegesis_from_proto_file=/path/to/sdm.pdf.pb \
//    exegesis_to_proto_file=/path/to/newer_sdm.pdf.pb \
//    exegesis_output_file_base=/tmp/newer_sdm_patches

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "exegesis/base/init_main.h"
#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"

ABSL_FLAG(std::string, exegesis_from_proto_file, "",
          "The path to the original PDF data in the format produced by "
          "//exegesis/tools:pdf2proto.");
ABSL_FLAG(std::string, exegesis_to_proto_file,
          "The path to the modified PDF data in the format produced by "
          "//exegesis/tools:pdf2proto.",
          "");
ABSL_FLAG(std::string, exegesis_output_file_base,
          "The base path for the files produced by the tool.", "");
ABSL_FLAG(
    std::string, exegesis_patches_directory, "exegesis/x86/pdf/sdm_patches/",
    "A folder containing a set of patches to apply to original documents");

namespace exegesis {
namespace pdf {
namespace {

std::string GetFilename(const std::string& name) {
  return absl::StrCat(absl::GetFlag(FLAGS_exegesis_output_file_base), "_", name,
                      ".pb.txt");
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
  const std::string exegesis_from_proto_file =
      absl::GetFlag(FLAGS_exegesis_from_proto_file);
  const std::string exegesis_to_proto_file =
      absl::GetFlag(FLAGS_exegesis_to_proto_file);
  const std::string exegesis_patches_directory =
      absl::GetFlag(FLAGS_exegesis_patches_directory);
  CHECK(!exegesis_from_proto_file.empty())
      << "missing --exegesis_from_proto_file";
  CHECK(!exegesis_to_proto_file.empty()) << "missing --exegesis_to_proto_file";
  CHECK(!exegesis_patches_directory.empty())
      << "missing --exegesis_patches_directory";
  CHECK(!absl::GetFlag(FLAGS_exegesis_output_file_base).empty())
      << "missing --exegesis_output_file_base";

  LOG(INFO) << "Opening original document " << exegesis_from_proto_file;
  const auto from_document =
      ReadBinaryProtoOrDie<PdfDocument>(exegesis_from_proto_file);
  LOG(INFO) << "Opening patches from " << exegesis_patches_directory;
  const auto patch_sets = LoadConfigurations(exegesis_patches_directory);
  LOG(INFO) << "Finding original patches";
  const auto& changes = FindPatchesOrDie(from_document, patch_sets);
  LOG(INFO) << "Checking patches";
  CheckPatchesOrDie(from_document, changes);
  LOG(INFO) << "Opening destination document " << exegesis_to_proto_file;
  const auto to_document =
      ReadBinaryProtoOrDie<PdfDocument>(exegesis_to_proto_file);

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
  exegesis::InitMain(argc, argv);
  ::exegesis::pdf::Main();
  return 0;
}
