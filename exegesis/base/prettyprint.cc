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

#include "exegesis/base/prettyprint.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/base/port_mask.h"
#include "exegesis/util/instruction_syntax.h"

namespace exegesis {

using ::google::protobuf::RepeatedPtrField;

std::string PrettyPrintCpuInfo(const CpuInfo& cpu_info,
                               const PrettyPrintOptions& options) {
  return absl::StrCat(cpu_info.proto().model_id(), " (name: '",
                      cpu_info.proto().code_name(), "')");
}

std::string PrettyPrintMicroArchitecture(
    const MicroArchitecture& microarchitecture,
    const PrettyPrintOptions& options) {
  std::string result = microarchitecture.proto().id();
  if (options.cpu_details) {
    absl::StrAppend(&result, "\nport masks:\n  ",
                    absl::StrJoin(microarchitecture.port_masks(), "\n  ",
                                  [](std::string* out, const PortMask& mask) {
                                    out->append(mask.ToString());
                                  }));
  }
  return result;
}

std::string PrettyPrintSyntax(const InstructionFormat& syntax) {
  return ConvertToCodeString(syntax);
}

std::string PrettyPrintSyntaxes(
    const RepeatedPtrField<InstructionFormat>& syntaxes,
    const PrettyPrintOptions& options) {
  const absl::string_view separator =
      options.vendor_syntaxes_on_one_line ? "; " : "\n";
  return absl::StrJoin(syntaxes, separator,
                       [](std::string* out, const InstructionFormat& syntax) {
                         out->append(PrettyPrintSyntax(syntax));
                       });
}

std::string PrettyPrintMicroOperation(const MicroOperationProto& uop,
                                      const PrettyPrintOptions& options) {
  std::string result = PortMask(uop.port_mask()).ToString();
  if (options.microop_latencies) {
    absl::StrAppend(&result, " (lat:", uop.latency(), ")");
  }
  if (!uop.dependencies().empty() && options.microop_dependencies) {
    absl::StrAppend(&result, " (deps:", absl::StrJoin(uop.dependencies(), ","),
                    ")");
  }
  return result;
}

std::string PrettyPrintInstruction(const InstructionProto& instruction,
                                   const PrettyPrintOptions& options) {
  std::string result =
      PrettyPrintSyntaxes(instruction.vendor_syntax(), options);
  if (!instruction.llvm_mnemonic().empty()) {
    absl::StrAppend(&result, "\nllvm: ", instruction.llvm_mnemonic(), "");
  }
  if (options.alternative_syntax) {
    if (instruction.has_syntax()) {
      absl::StrAppend(&result,
                      "\nintel: ", PrettyPrintSyntax(instruction.syntax()));
    }
    if (instruction.has_att_syntax()) {
      absl::StrAppend(&result,
                      "\natt: ", PrettyPrintSyntax(instruction.att_syntax()));
    }
  }
  return result;
}

std::string PrettyPrintItinerary(const ItineraryProto& itineraries,
                                 const PrettyPrintOptions& options) {
  std::string result;
  if (!itineraries.micro_ops().empty()) {
    absl::StrAppend(
        &result, options.itineraries_on_one_line ? "" : "  ",
        absl::StrJoin(
            itineraries.micro_ops(),
            options.itineraries_on_one_line ? " " : "\n  ",
            [&options](std::string* out, const MicroOperationProto& uop) {
              out->append(PrettyPrintMicroOperation(uop, options));
            }));
  }
  return result;
}

}  // namespace exegesis
