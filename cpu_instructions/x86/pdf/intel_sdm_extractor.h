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

#ifndef CPU_INSTRUCTIONS_X86_PDF_INTEL_SDM_EXTRACTOR_H_
#define CPU_INSTRUCTIONS_X86_PDF_INTEL_SDM_EXTRACTOR_H_

#include "strings/string.h"

#include "cpu_instructions/proto/instructions.pb.h"
#include "cpu_instructions/x86/pdf/intel_sdm.pb.h"
#include "cpu_instructions/x86/pdf/pdf_document.pb.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

void ProcessIntelPdfDocument(PdfDocument* document, SdmDocument* sdm_document);

void ProcessIntelSdmDocument(SdmDocument* sdm_document,
                             InstructionSetProto* instruction_set);

// Parses the contents of an operand encoding cell.
InstructionTable::OperandEncodingCrossref::OperandEncoding
ParseOperandEncodingTableCell(const string& content);

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_PDF_INTEL_SDM_EXTRACTOR_H_
