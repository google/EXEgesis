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

#ifndef EXEGESIS_BASE_DEBUG_H_
#define EXEGESIS_BASE_DEBUG_H_

#include "exegesis/base/cpu_info.h"
#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "src/google/protobuf/repeated_field.h"

namespace exegesis {

struct PrettyPrintOptions {
 public:
  PrettyPrintOptions() {}

  // If true, print CPU models details. Else just print the model
  // identification information.
  PrettyPrintOptions& WithCpuDetails(bool v) {
    cpu_details = v;
    return *this;
  }

  // If true, print Intel and AT&T syntaxes.
  PrettyPrintOptions& WithAlternativeSyntax(bool v) {
    alternative_syntax = v;
    return *this;
  }

  // If true, prints itineraries on a single line instead of one per line.
  PrettyPrintOptions& WithItinerariesOnOneLine(bool v) {
    itineraries_on_one_line = v;
    return *this;
  }

  // If false, do not print micro-op latencies.
  PrettyPrintOptions& WithMicroOpLatencies(bool v) {
    microop_latencies = v;
    return *this;
  }

  // If false, do not print micro-op dependencies.
  PrettyPrintOptions& WithMicroOpDependencies(bool v) {
    microop_dependencies = v;
    return *this;
  }

  PrettyPrintOptions& WithVendorSyntaxesOnOneLine(bool v) {
    vendor_syntaxes_on_one_line = v;
    return *this;
  }

  bool vendor_syntaxes_on_one_line = false;
  bool cpu_details = false;
  bool alternative_syntax = false;
  bool itineraries_on_one_line = false;
  bool microop_latencies = true;
  bool microop_dependencies = true;
};

std::string PrettyPrintInstruction(
    const InstructionProto& instruction,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintItinerary(
    const ItineraryProto& itineraries,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintCpuInfo(
    const CpuInfo& cpu_info,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintMicroArchitecture(
    const MicroArchitecture& microarchitecture,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintSyntax(const InstructionFormat& syntax);

std::string PrettyPrintSyntaxes(
    const ::google::protobuf::RepeatedPtrField<InstructionFormat>& syntaxes,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintMicroOperation(
    const MicroOperationProto& uop,
    const PrettyPrintOptions& options = PrettyPrintOptions());

std::string PrettyPrintMicroOperations(
    const ::google::protobuf::RepeatedPtrField<MicroOperationProto>& uops,
    const PrettyPrintOptions& options = PrettyPrintOptions());

}  // namespace exegesis

#endif  // EXEGESIS_BASE_DEBUG_H_
