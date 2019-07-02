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
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "exegesis/base/init_main.h"
#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"

ABSL_FLAG(std::string, exegesis_proto_input_file, "",
          "Path to the binary proto representation of the PDF file.");
ABSL_FLAG(std::string, exegesis_match_expression, "",
          "The regular expression to match cells to patch.");
ABSL_FLAG(std::string, exegesis_page_numbers, "",
          "A list of page numbers to process, all pages if not set.");

namespace exegesis {
namespace pdf {
namespace {

bool ShouldProcessTextBlock(const PdfTextBlock& block) {
  static const std::string* const match_expression =
      new std::string(absl::GetFlag(FLAGS_exegesis_match_expression));
  if (match_expression->empty()) return true;
  return RE2::PartialMatch(block.text(), *match_expression);
}

bool ShouldProcessPage(const absl::flat_hash_set<size_t>& allowed_pages,
                       const PdfPage& page) {
  return allowed_pages.empty() || allowed_pages.contains(page.number());
}

absl::flat_hash_set<size_t> ParsePageNumbers() {
  absl::flat_hash_set<size_t> pages;
  int page_number = 0;
  const std::string input_value = absl::GetFlag(FLAGS_exegesis_page_numbers);
  absl::string_view input(input_value);
  while (RE2::Consume(&input, "(\\d+),?", &page_number)) {
    pages.insert(page_number);
  }
  CHECK(input.empty()) << "Can't parse page number '" << input << "'";
  return pages;
}

void Main() {
  const std::string exegesis_proto_input_file =
      absl::GetFlag(FLAGS_exegesis_proto_input_file);
  CHECK(!exegesis_proto_input_file.empty())
      << "missing --exegesis_proto_input_file";
  CHECK(!absl::GetFlag(FLAGS_exegesis_match_expression).empty())
      << "missing --exegesis_match_expression";

  const auto pdf_document =
      ReadBinaryProtoOrDie<PdfDocument>(exegesis_proto_input_file);

  // Prepare patches.
  const auto pages = ParsePageNumbers();
  std::map<size_t, std::vector<PdfPagePatch>> page_patches;
  for (const auto& page : pdf_document.pages()) {
    if (!ShouldProcessPage(pages, page)) continue;

    for (const auto& row : page.rows()) {
      for (const auto& block : row.blocks()) {
        if (ShouldProcessTextBlock(block)) {
          PdfPagePatch patch;
          patch.set_row(block.row());
          patch.set_col(block.col());
          patch.set_expected(block.text());
          patch.set_replacement(block.text());
          page_patches[page.number()].push_back(patch);
        }
      }
    }
  }

  // Gather patches per page.
  PdfDocumentsChanges documents_changes;
  auto* document_changes = documents_changes.add_documents();
  *document_changes->mutable_document_id() = pdf_document.document_id();
  for (const auto& page_patches_pair : page_patches) {
    auto* page_patches = document_changes->add_pages();
    page_patches->set_page_number(page_patches_pair.first);
    const auto& patches = page_patches_pair.second;
    std::copy(patches.begin(), patches.end(),
              google::protobuf::RepeatedFieldBackInserter(
                  page_patches->mutable_patches()));
  }

  // Display patches.
  documents_changes.PrintDebugString();
}

}  // namespace
}  // namespace pdf
}  // namespace exegesis

int main(int argc, char** argv) {
  exegesis::InitMain(argc, argv);
  ::exegesis::pdf::Main();
  return 0;
}
