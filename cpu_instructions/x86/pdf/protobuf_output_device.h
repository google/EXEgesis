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

#ifndef CPU_INSTRUCTIONS_X86_PDF_PROTOBUF_OUTPUT_DEVICE_H_
#define CPU_INSTRUCTIONS_X86_PDF_PROTOBUF_OUTPUT_DEVICE_H_

#include <functional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "xpdf-3.04/xpdf/GfxState.h"
#include "xpdf-3.04/xpdf/OutputDev.h"
#include "cpu_instructions/x86/pdf/pdf_document.pb.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

// An XPDF device which outputs the stream of characters as a PdfDocument
// protobuf.
class ProtobufOutputDevice : public OutputDev {
 public:
  // PdfDocumentChanges is used to change the way the document is parsed, it is
  // also responsible for patching the document afterwards.
  // ProtobufOutputDevice does not acquire ownership of pdf_document.
  // pdf_document should outlive this instance.
  ProtobufOutputDevice(const PdfDocumentChanges& document_changes,
                       PdfDocument* pdf_document);
  ProtobufOutputDevice(const ProtobufOutputDevice&) = delete;
  ~ProtobufOutputDevice() override;

 private:
  GBool upsideDown() override { return gTrue; }
  GBool useDrawChar() override { return gTrue; }
  GBool interpretType3Chars() override { return gFalse; }
  GBool needNonText() override { return gFalse; }

  void startPage(int pageNum, GfxState* state) override;
  void endPage() override;
  void drawChar(GfxState* state, double x, double y, double dx, double dy,
                double originX, double originY, CharCode c, int nBytes,
                Unicode* u, int uLen) override;

  const PdfDocumentChanges document_changes_;
  PdfDocument* const pdf_document_ = nullptr;
  PdfPage current_page_;
};

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_PDF_PROTOBUF_OUTPUT_DEVICE_H_
