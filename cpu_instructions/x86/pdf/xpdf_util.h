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

#ifndef CPU_INSTRUCTIONS_X86_PDF_XPDF_UTIL_H_
#define CPU_INSTRUCTIONS_X86_PDF_XPDF_UTIL_H_

#include <map>
#include <memory>
#include "strings/string.h"

#include "cpu_instructions/x86/pdf/pdf_document.pb.h"

// xpdf classes.
class PDFDoc;

namespace cpu_instructions {
namespace x86 {
namespace pdf {

// Represents an XPDF document.
class XPDFDoc {
 public:
  typedef std::map<string, string> Metadata;

  static std::unique_ptr<const XPDFDoc> OpenOrDie(const string& filename);

  ~XPDFDoc();

  const Metadata& GetMetadata() const { return metadata_; }
  const PdfDocumentId& GetDocumentId() const { return doc_id_; }

  PdfDocument Parse(int first_page, int last_page,
                    const PdfDocumentChanges& patches) const;

 private:
  explicit XPDFDoc(std::unique_ptr<PDFDoc> doc);

  std::unique_ptr<PDFDoc> doc_;
  const Metadata metadata_;
  const PdfDocumentId doc_id_;
};

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_PDF_XPDF_UTIL_H_
