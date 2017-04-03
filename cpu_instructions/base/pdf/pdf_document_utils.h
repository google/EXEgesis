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

// This file provides primitives to compose to extract parts of a PdfDocument.
#ifndef CPU_INSTRUCTIONS_BASE_PDF_PDF_DOCUMENT_UTILS_H_
#define CPU_INSTRUCTIONS_BASE_PDF_PDF_DOCUMENT_UTILS_H_

#include "strings/string.h"

#include "cpu_instructions/base/pdf/pdf_document.pb.h"
#include "strings/string_view.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

// Returns the text for the corresponding cell in the page or empty string.
// row and col indices can be negative to indicate reverse order.
// e.g. row = -1 means 'last row of the page'.
// e.g. col = -1 means 'last column of the row'.
const string& GetCellTextOrEmpty(const PdfPage& page, int row, int col);

// Mutable version of the above function.
// Returns nullptr if cell does not exist.
string* GetMutableCellTextOrNull(PdfPage* page, int row, int col);

// Returns the text for the corresponding cell in the row or empty string.
// col index can be negative to indicate reverse order.
// e.g. col = -1 means 'last column of row'.
const string& GetCellTextOrEmpty(const PdfTextTableRow& row, int col);

// Mutable version of the above function.
// Returns nullptr if cell does not exist.
string* GetMutableCellTextOrNull(PdfTextTableRow* row, int col);

// Applies patches to the page.
void ApplyPatchOrDie(const PdfPagePatch& patch, PdfPage* page);

// Retrieve the page's rows excluding header and footer.
std::vector<const PdfTextTableRow*> GetPageBodyRows(const PdfPage& page,
                                                    float margin);

// Returns the changes corresponding to the given document id, or nullptr if not
// found.
const PdfDocumentChanges* GetConfigOrNull(const PdfDocumentsChanges& patch_sets,
                                          const PdfDocumentId& document_id);
}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_BASE_PDF_PDF_DOCUMENT_UTILS_H_
