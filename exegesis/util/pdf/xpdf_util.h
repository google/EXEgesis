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

// Utilities to manipulate PDF files with xpdf.

#ifndef EXEGESIS_PDF_PARSING_XPDF_UTIL_H_
#define EXEGESIS_PDF_PARSING_XPDF_UTIL_H_

#include <map>
#include <memory>
#include <string>

#include "exegesis/proto/pdf/pdf_document.pb.h"

namespace exegesis {
namespace pdf {

// Parses a string of the form <path/to/filename>(:<first>-<last>)?
// e.g. "/path/to/file.pdf"
// e.g. "/path/to/file.pdf:12-25"
PdfParseRequest ParseRequestOrDie(const std::string& spec);

// Parses a PDF file described by a PdfParseEntry.
//
// The function will:
// - open the file,
// - read the metadata,
// - build a document id from the metadata,
// - look in documents_patches for a corresponding set of changes,
// - parse the pdf and apply the changes,
// - return the corresponding PdfDocument.
//
// Please note that documents_patches have to contains an entry for the pdf's
// document id or the function will die. Leave documents_patches empty for
// tests.
PdfDocument ParseOrDie(const PdfParseRequest& request,
                       const PdfDocumentsChanges& documents_patches);

}  // namespace pdf
}  // namespace exegesis

#endif  // EXEGESIS_PDF_PARSING_XPDF_UTIL_H_
