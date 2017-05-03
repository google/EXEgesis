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

#include "cpu_instructions/base/prettyprint.h"

#include "cpu_instructions/base/port_mask.h"
#include "strings/str_cat.h"
#include "strings/str_join.h"

namespace cpu_instructions {

string PrettyPrintCpuModel(const CpuModel& cpu_model,
                           const PrettyPrintOptions& options) {
  string result = StrCat(cpu_model.proto().id(), " (name: '",
                         cpu_model.proto().code_name(), "')");
  if (options.cpu_details_) {
    StrAppend(&result, "\nport masks:\n  ",
              strings::Join(cpu_model.microarchitecture().port_masks(), "\n  ",
                            [](string* out, const PortMask& mask) {
                              out->append(mask.ToString());
                            }));
  }
  return result;
}

string PrettyPrintSyntax(const InstructionFormat& syntax,
                         const PrettyPrintOptions& options) {
  string result = syntax.mnemonic();
  if (!syntax.operands().empty()) {
    StrAppend(&result, " ",
              strings::Join(syntax.operands(), ", ",
                            [](string* out, const InstructionOperand& operand) {
                              out->append(operand.name());
                            }));
  }
  return result;
}

string PrettyPrintMicroOperation(const MicroOperationProto& uop,
                                 const PrettyPrintOptions& options) {
  string result = PortMask(uop.port_mask()).ToString();
  if (uop.has_latency() && options.microop_latencies_) {
    StrAppend(&result, " (lat:", uop.latency(), ")");
  }
  if (!uop.dependencies().empty() && options.microop_dependencies_) {
    StrAppend(&result, " (deps:", strings::Join(uop.dependencies(), ","), ")");
  }
  return result;
}

string PrettyPrintInstruction(const InstructionProto& instruction,
                              const PrettyPrintOptions& options) {
  string result = StrCat(PrettyPrintSyntax(instruction.vendor_syntax()));
  if (instruction.has_llvm_mnemonic()) {
    StrAppend(&result, "\nllvm: ", instruction.llvm_mnemonic(), "");
  }
  if (options.alternative_syntax_) {
    if (instruction.has_syntax()) {
      StrAppend(&result, "\nintel: ", PrettyPrintSyntax(instruction.syntax()));
    }
    if (instruction.has_att_syntax()) {
      StrAppend(&result,
                "\natt: ", PrettyPrintSyntax(instruction.att_syntax()));
    }
  }
  return result;
}

string PrettyPrintItinerary(const ItineraryProto& itineraries,
                            const PrettyPrintOptions& options) {
  string result;
  if (!itineraries.micro_ops().empty()) {
    StrAppend(
        &result, options.itineraries_on_one_line_ ? "" : "  ",
        strings::Join(itineraries.micro_ops(),
                      options.itineraries_on_one_line_ ? " " : "\n  ",
                      [&options](string* out, const MicroOperationProto& uop) {
                        out->append(PrettyPrintMicroOperation(uop, options));
                      }));
  }
  return result;
}

}  // namespace cpu_instructions
