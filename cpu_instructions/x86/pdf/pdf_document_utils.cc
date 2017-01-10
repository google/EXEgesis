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

#include "cpu_instructions/x86/pdf/pdf_document_utils.h"

#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

namespace {

// Positive, in bound is simply itselt
//   GetIndex(5, 1) => 1
// Negative starts from the end of the array
//   GetIndex(5, -1) => 4
// Invalid index yields -1
//  GetIndex(5, 10) => -1
int GetIndex(int size, int index) {
  CHECK_GE(size, 0);
  if (index >= 0 && index >= size) return -1;
  if (index < 0 && -index > size) return -1;
  return index >= 0 ? index : size + index;
}

}  // namespace

const string& GetCellTextOrEmpty(const PdfPage& page, int row, int col) {
  static const string kEmpty;
  const int row_index = GetIndex(page.rows_size(), row);
  if (row_index < 0) return kEmpty;
  return GetCellTextOrEmpty(page.rows(row_index), col);
}

const string& GetCellTextOrEmpty(const PdfTextTableRow& row, int col) {
  static const string kEmpty;
  const int col_index = GetIndex(row.blocks_size(), col);
  if (col_index < 0) return kEmpty;
  return row.blocks(col_index).text();
}

string* GetMutableCellTextOrNull(PdfPage* page, int row, int col) {
  const int row_index = GetIndex(page->rows_size(), row);
  if (row_index < 0) return nullptr;
  return GetMutableCellTextOrNull(page->mutable_rows(row_index), col);
}

string* GetMutableCellTextOrNull(PdfTextTableRow* row, int col) {
  const int col_index = GetIndex(row->blocks_size(), col);
  if (col_index < 0) return nullptr;
  return row->mutable_blocks(col_index)->mutable_text();
}

void ApplyPatchOrDie(const PdfPagePatch& patch, PdfPage* page) {
  string* text = GetMutableCellTextOrNull(page, patch.row(), patch.col());
  CHECK(text != nullptr) << "No valid cell for patch "
                         << patch.ShortDebugString();
  CHECK_EQ(*text, patch.expected()) << "Can't apply patch, unexpected value";
  *text = patch.replacement();
}

std::vector<const PdfTextTableRow*> GetPageBodyRows(const PdfPage& page,
                                                    const float margin) {
  const float top_margin = margin;
  const float bottom_margin = page.height() - margin;
  std::vector<const PdfTextTableRow*> result;
  for (const auto& row : page.rows()) {
    if (row.bounding_box().top() > top_margin &&
        row.bounding_box().bottom() < bottom_margin)
      result.push_back(&row);
  }
  return result;
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
}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
