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

#include "exegesis/base/cpu_model.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"

namespace exegesis {

class PrettyPrintOptions {
 public:
  PrettyPrintOptions() {}

  // If true, print CPU models details. Else just print the model
  // identification information.
  PrettyPrintOptions& WithCpuDetails(bool v) {
    cpu_details_ = v;
    return *this;
  }

  // If true, print Intel and AT&T syntaxes.
  PrettyPrintOptions& WithAlternativeSyntax(bool v) {
    alternative_syntax_ = v;
    return *this;
  }

  // If true, prints itineraries on a single line instead of one per line.
  PrettyPrintOptions& WithItinerariesOnOneLine(bool v) {
    itineraries_on_one_line_ = v;
    return *this;
  }

  // If false, do not print micro-op latencies.
  PrettyPrintOptions& WithMicroOpLatencies(bool v) {
    microop_latencies_ = v;
    return *this;
  }

  // If false, do not print micro-op dependencies.
  PrettyPrintOptions& WithMicroOpDependencies(bool v) {
    microop_dependencies_ = v;
    return *this;
  }

  bool cpu_details_ = false;
  bool alternative_syntax_ = false;
  bool itineraries_on_one_line_ = false;
  bool microop_latencies_ = true;
  bool microop_dependencies_ = true;
};

string PrettyPrintInstruction(
    const InstructionProto& instruction,
    const PrettyPrintOptions& options = PrettyPrintOptions());

string PrettyPrintItinerary(
    const ItineraryProto& instruction,
    const PrettyPrintOptions& options = PrettyPrintOptions());

string PrettyPrintCpuModel(
    const CpuModel& cpu_model,
    const PrettyPrintOptions& options = PrettyPrintOptions());

string PrettyPrintSyntax(
    const InstructionFormat& syntax,
    const PrettyPrintOptions& options = PrettyPrintOptions());

string PrettyPrintMicroOperation(
    const MicroOperationProto& uop,
    const PrettyPrintOptions& options = PrettyPrintOptions());

string PrettyPrintMicroOperations(
    const ::google::protobuf::RepeatedPtrField<MicroOperationProto>& uops,
    const PrettyPrintOptions& options = PrettyPrintOptions());

}  // namespace exegesis

#endif  // EXEGESIS_BASE_DEBUG_H_
