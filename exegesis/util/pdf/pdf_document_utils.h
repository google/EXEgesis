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
#ifndef EXEGESIS_UTIL_PDF_PDF_DOCUMENT_UTILS_H_
#define EXEGESIS_UTIL_PDF_PDF_DOCUMENT_UTILS_H_

#include <string>

#include "absl/strings/string_view.h"
#include "exegesis/proto/pdf/pdf_document.pb.h"

namespace exegesis {
namespace pdf {

// Returns the corresponding cell in the page or nullptr.
// row and col indices can be negative to indicate reverse order.
// e.g. row = -1 means 'last row of the page'.
// e.g. col = -1 means 'last column of the row'.
const PdfTextBlock* GetCellOrNull(const PdfPage& page, int row, int col);

// Returns the text for the corresponding cell in the page or empty string.
// row and col indices can be negative to indicate reverse order.
// e.g. row = -1 means 'last row of the page'.
// e.g. col = -1 means 'last column of the row'.
const std::string& GetCellTextOrEmpty(const PdfPage& page, int row, int col);

// Mutable version of the above function.
// Returns nullptr if cell does not exist.
std::string* GetMutableCellTextOrNull(PdfPage* page, int row, int col);

// Mutable version of the above function.
// Returns nullptr if cell does not exist.
std::string* GetMutableCellTextOrNull(PdfTextTableRow* row, int col);

// Applies patches to the page.
void ApplyPatchOrDie(const PdfPagePatch& patch, PdfPage* page);

// Returns true if the patch applies successfully.
bool CheckPatch(const PdfPagePatch& patch, const PdfPage& page);

// Retrieve the page's rows excluding header and footer.
// If max_row is not set (or is set to negative), returns the whole table,
// otherwise, returns the first max_row rows.
std::vector<const PdfTextTableRow*> GetPageBodyRows(const PdfPage& page,
                                                    float margin,
                                                    int max_row = -1);

// Loads all files in directory and returns the merged PdfDocumentsChanges.
PdfDocumentsChanges LoadConfigurations(const std::string& directory);

// Returns the changes corresponding to the given document id, or nullptr if not
// found.
const PdfDocumentChanges* GetConfigOrNull(const PdfDocumentsChanges& patch_sets,
                                          const PdfDocumentId& document_id);

// Tries to apply patches from one document to another.
// 'successful_patches' and 'failed_patches' must not be nullptr.
//
// The algorithm computes a vector of hashes of PdfTextBlock from both documents
// and finds subsequences of hashes that matches between the two. Then we
// traverse the matching ranges starting by longest matches to get a mapping
// from block in the from_document to block in the to_document.
//
// Longest matching subsequence algorithm is described here:
// https://cs.stackexchange.com/a/9619
//
// If a patch is part of this mapping we rewrite it using the destination block
// coordinates and append it to successful_patches, otherwise we append it to
// failed_patches.
void TransferPatches(const PdfDocumentChanges& changes,
                     const PdfDocument& from_document,
                     const PdfDocument& to_document,
                     PdfDocumentChanges* successful_patches,
                     PdfDocumentChanges* failed_patches);
}  // namespace pdf
}  // namespace exegesis

#endif  // EXEGESIS_UTIL_PDF_PDF_DOCUMENT_UTILS_H_
