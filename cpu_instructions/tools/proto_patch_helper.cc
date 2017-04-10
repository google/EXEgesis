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
// cpu_instructions/tools:proto_patch_helper -- \
// --cpu_instructions_proto_input_file=/path/to/sdm.pdf.pb \
// --cpu_instructions_match_expression='SAL/SAR/SHL/SHR' \
// --cpu_instruction_page_numbers=662

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "strings/string.h"

#include "gflags/gflags.h"

#include "cpu_instructions/proto/pdf/pdf_document.pb.h"
#include "cpu_instructions/util/pdf/xpdf_util.h"
#include "cpu_instructions/util/proto_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_split.h"
#include "util/gtl/map_util.h"

DEFINE_string(cpu_instructions_proto_input_file, "",
              "Path to the binary proto representation of the PDF file.");
DEFINE_string(cpu_instructions_match_expression, "",
              "The regular expression to match cells to patch.");
DEFINE_string(cpu_instruction_page_numbers, "",
              "A list of page numbers to process, all pages if not set.");

namespace cpu_instructions {
namespace pdf {
namespace {

bool ShouldProcessTextBlock(const PdfTextBlock& block) {
  if (FLAGS_cpu_instructions_match_expression.empty()) return true;
  return RE2::PartialMatch(block.text(),
                           FLAGS_cpu_instructions_match_expression);
}

bool ShouldProcessPage(const std::unordered_set<size_t>& allowed_pages,
                       const PdfPage& page) {
  if (allowed_pages.empty()) return true;
  return ContainsKey(allowed_pages, page.number());
}

std::unordered_set<size_t> ParsePageNumbers() {
  std::unordered_set<size_t> pages;
  int page_number = 0;
  ::re2::

      StringPiece input(FLAGS_cpu_instruction_page_numbers);
  while (RE2::Consume(&input, "(\\d+),?", &page_number)) {
    pages.insert(page_number);
  }
  CHECK(input.empty()) << "Can't parse page number '" << input << "'";
  return pages;
}

void Main() {
  CHECK(!FLAGS_cpu_instructions_proto_input_file.empty())
      << "missing --cpu_instructions_proto_input_file";
  CHECK(!FLAGS_cpu_instructions_match_expression.empty())
      << "missing --cpu_instructions_match_expression";

  const auto pdf_document = ReadBinaryProtoOrDie<PdfDocument>(
      FLAGS_cpu_instructions_proto_input_file);

  // Prepare patches.
  const auto pages = ParsePageNumbers();
  std::map<size_t, std::vector<PdfPagePatch>> page_patches;
  for (const auto& page : pdf_document.pages()) {
    if (!ShouldProcessPage(pages, page)) continue;

    size_t row_index = 0;
    for (const auto& row : page.rows()) {
      size_t col_index = 0;
      for (const auto& block : row.blocks()) {
        if (ShouldProcessTextBlock(block)) {
          PdfPagePatch patch;
          patch.set_row(row_index);
          patch.set_col(col_index);
          patch.set_expected(block.text());
          patch.set_replacement("CHANGE ME");
          page_patches[page.number()].push_back(patch);
        }
        ++col_index;
      }
      ++row_index;
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
}  // namespace cpu_instructions

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  ::cpu_instructions::pdf::Main();
  return 0;
}
