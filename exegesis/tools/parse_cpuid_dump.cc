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

// Parses CPUID dumps in the supported formats, and converts them to the formats
// understood by EXEgesis tools.
//
// Example:
//  parse_cpuid_dump --exegesis_input_x86_cpuid_dump=- \
//      --exegesis_output_cpu_model=-

#include <algorithm>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "exegesis/base/cpuid.h"
#include "exegesis/base/cpuid_x86.h"
#include "exegesis/base/init_main.h"
#include "exegesis/proto/cpuid.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "exegesis/util/file_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

// Supported input formats.
ABSL_FLAG(
    std::string, exegesis_input_x86_cpuid_dump, "",
    "The name of the file from which the x86 CPUID dump is read or - "
    "to read from stdin. The dump must be in the text format supported by "
    "exegesis::CpuIdDump::FromString().");
ABSL_FLAG(std::string, exegesis_input_cpuid_dump_proto, "",
          "The name of the file from which a CpuIdDumpProto is read or - "
          "to read from stdin. The proto must be in the text format.");

// Supported output formats.
ABSL_FLAG(std::string, exegesis_output_cpu_model, "",
          "The name of the file to which the CPU model information is "
          "written or - to write to stdout.");
ABSL_FLAG(std::string, exegesis_output_cpuid_dump, "",
          "The name of the file to which the CPUID dump is written as a "
          "proto in text format, or - to write to stdout.");

namespace exegesis {
namespace {

using ::exegesis::util::StatusOr;
using ::google::protobuf::RepeatedFieldBackInserter;
using ::google::protobuf::RepeatedPtrField;

CpuIdDumpProto ParseX86CpuIdDumpOrDie(const std::string& input) {
  const std::string cpuid_dump_text = ReadTextFromFileOrStdInOrDie(input);
  const StatusOr<x86::CpuIdDump> dump_or_status =
      x86::CpuIdDump::FromString(cpuid_dump_text);
  CHECK_OK(dump_or_status.status());
  return dump_or_status.ValueOrDie().dump_proto();
}

CpuIdDumpProto ParseCpuIdDumpProtoOrDie(const std::string& input) {
  const std::string text_proto = ReadTextFromFileOrStdInOrDie(input);
  return ParseProtoFromStringOrDie<CpuIdDumpProto>(text_proto);
}

void PrintCpuIdDump(const CpuIdDumpProto& cpuid_dump,
                    const std::string& output) {
  WriteTextToFileOrStdOutOrDie(output, cpuid_dump.DebugString());
}

void PrintCpuModelFromCpuIdDump(const CpuIdDumpProto& cpuid_dump,
                                const std::string& output) {
  const CpuInfo cpu_info = CpuInfoFromCpuIdDump(cpuid_dump);
  CpuInfoProto cpu_info_proto;
  cpu_info_proto.set_model_id(cpu_info.cpu_model_id());
  RepeatedPtrField<std::string>* const features =
      cpu_info_proto.mutable_feature_names();
  std::copy(cpu_info.supported_features().begin(),
            cpu_info.supported_features().end(),
            RepeatedFieldBackInserter(features));
  std::sort(features->begin(), features->end());
  WriteTextToFileOrStdOutOrDie(output, cpu_info_proto.DebugString());
}

void ProcessCpuIdDump() {
  CpuIdDumpProto cpuid_dump;
  const std::string exegesis_input_x86_cpuid_dump =
      absl::GetFlag(FLAGS_exegesis_input_x86_cpuid_dump);
  const std::string exegesis_input_cpuid_dump_proto =
      absl::GetFlag(FLAGS_exegesis_input_cpuid_dump_proto);
  if (!exegesis_input_x86_cpuid_dump.empty()) {
    cpuid_dump = ParseX86CpuIdDumpOrDie(exegesis_input_x86_cpuid_dump);
  } else if (!exegesis_input_cpuid_dump_proto.empty()) {
    cpuid_dump = ParseCpuIdDumpProtoOrDie(exegesis_input_cpuid_dump_proto);
  } else {
    LOG(FATAL) << "No CPUID dump source was specified.";
  }

  const std::string exegesis_output_cpu_model =
      absl::GetFlag(FLAGS_exegesis_output_cpu_model);
  if (!exegesis_output_cpu_model.empty()) {
    PrintCpuModelFromCpuIdDump(cpuid_dump, exegesis_output_cpu_model);
  }
  const std::string exegesis_output_cpuid_dump =
      absl::GetFlag(FLAGS_exegesis_output_cpuid_dump);
  if (!exegesis_output_cpuid_dump.empty()) {
    PrintCpuIdDump(cpuid_dump, exegesis_output_cpuid_dump);
  }
}

}  // namespace
}  // namespace exegesis

int main(int argc, char* argv[]) {
  exegesis::InitMain(argc, argv);
  exegesis::ProcessCpuIdDump();
  return 0;
}
