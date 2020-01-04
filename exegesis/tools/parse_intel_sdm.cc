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

#include <string>

#include "absl/flags/flag.h"
#include "absl/strings/str_cat.h"
#include "exegesis/base/architecture.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/base/init_main.h"
#include "exegesis/base/transform_factory.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/x86/pdf/parse_sdm.h"
#include "glog/logging.h"
#include "util/task/status.h"

ABSL_FLAG(std::string, exegesis_input_spec, "",
          "Input spec: List of files and ranges to process in the form "
          " filename or filename:start-end, e.g. "
          "'file1.pdf:83-86,file1.pdf:89-0,file2.pdf:1-50'. "
          "Ranges are 1-based and inclusive. The upper bound can be 0 to "
          "process all the pages to the end. If no range is provided, "
          "the entire PDF is processed. Required.");
ABSL_FLAG(std::string, exegesis_output_file_base, "",
          "Where to dump instructions. Required. The binary will write an "
          "ArchitectureProto with all instructions parsed form the manual "
          "to {exegesis_output_file_base}.pbtxt. When "
          "exegesis_parse_sm_store_intermediate_files is true, it will "
          "also store files with the intermediate data in other files with "
          "the same base name and different suffixes.");
ABSL_FLAG(
    std::string, exegesis_patches_directory, "exegesis/x86/pdf/sdm_patches/",
    "A folder containing a set of patches to apply to original documents");
ABSL_FLAG(bool, exegesis_ignore_failing_transforms, false,
          "Set if some transforms are failing but you still need to process "
          "the instruction set");

namespace exegesis {
namespace {

void Main() {
  const std::string exegesis_input_spec =
      absl::GetFlag(FLAGS_exegesis_input_spec);
  const std::string exegesis_output_file_base =
      absl::GetFlag(FLAGS_exegesis_output_file_base);
  CHECK(!exegesis_input_spec.empty()) << "missing --exegesis_input_spec";
  CHECK(!exegesis_output_file_base.empty())
      << "missing --exegesis_output_file_base";

  ArchitectureProto architecture = x86::pdf::ParseSdmOrDie(
      exegesis_input_spec, absl::GetFlag(FLAGS_exegesis_patches_directory),
      exegesis_output_file_base);

  // Optionally apply transforms in --exegesis_transforms.
  const auto result_status =
      RunTransformPipeline(GetTransformsFromCommandLineFlags(),
                           architecture.mutable_instruction_set());
  if (!absl::GetFlag(FLAGS_exegesis_ignore_failing_transforms)) {
    CHECK_OK(result_status);
  } else {
    LOG_IF(ERROR, !result_status.ok()) << result_status;
  }

  // Write transformed instruction set.
  const std::string architecture_filename =
      absl::StrCat(exegesis_output_file_base, ".pbtxt");
  LOG(INFO) << "Saving ArchitectureProto as: " << architecture_filename;
  WriteTextProtoOrDie(architecture_filename, architecture,
                      *GetArchitectureProtoTextPrinter());
}

}  // namespace
}  // namespace exegesis

int main(int argc, char** argv) {
  FLAGS_logtostderr = true;
  exegesis::InitMain(argc, argv);
  ::exegesis::Main();
  return EXIT_SUCCESS;
}
