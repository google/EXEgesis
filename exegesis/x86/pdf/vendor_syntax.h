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

#ifndef EXEGESIS_X86_PDF_VENDOR_SYNTAX_H_
#define EXEGESIS_X86_PDF_VENDOR_SYNTAX_H_

#include <string>

#include "exegesis/proto/instructions.pb.h"

namespace exegesis {
namespace x86 {
namespace pdf {

constexpr const char kUnknown[] = "<UNKNOWN>";

// Parses the vendor syntax (e.g. "ADC r/m16, imm8").
bool ParseVendorSyntax(std::string content,
                       InstructionFormat* instruction_format);

// This function is used to create a stable id from instruction name found:
// - at the top of a page describing a new instruction
// - in the footer of a page for a particular instruction
// It does so by removing some characters and imposing a limit on the text size.
// Limiting the size is necessary because when text is too long it gets
// truncated in different ways.
std::string NormalizeName(std::string text);

}  // namespace pdf
}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_PDF_VENDOR_SYNTAX_H_
