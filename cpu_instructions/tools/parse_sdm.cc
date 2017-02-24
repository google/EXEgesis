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

#include "strings/string.h"

#include "gflags/gflags.h"

#include "cpu_instructions/base/transform_factory.h"
#include "cpu_instructions/proto/instructions.pb.h"
#include "cpu_instructions/x86/pdf/parse_sdm.h"
#include "cpu_instructions/x86/pdf/proto_util.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "util/task/status.h"

DEFINE_string(cpu_instructions_input_spec, "",
              "Input spec: List of files and ranges to process in the form "
              " filename or filename:start-end, e.g. "
              "'file1.pdf:83-86,file1.pdf:89-0,file2.pdf:1-50'. "
              "Ranges are 1-based and inclusive. The upper bound can be 0 to "
              "process all the pages to the end. If no range is provided, "
              "the entire PDF is processed.");
DEFINE_string(cpu_instructions_output_file_base, "",
              "Where to dump instructions");
DEFINE_string(cpu_instructions_patch_sets_file,
              "cpu_instructions/x86/pdf/sdm_patches.pbtxt",
              "A set of patches to original documents");

namespace cpu_instructions {
namespace {

void Main() {
  CHECK(!FLAGS_cpu_instructions_input_spec.empty())
      << "missing --cpu_instructions_input_spec";
  CHECK(!FLAGS_cpu_instructions_output_file_base.empty())
      << "missing --cpu_instructions_output_file_base";

  InstructionSetProto instruction_set = x86::pdf::ParseSdmOrDie(
      FLAGS_cpu_instructions_input_spec, FLAGS_cpu_instructions_patch_sets_file,
      FLAGS_cpu_instructions_output_file_base);

  // Optionally apply transforms in --cpu_instructions_transforms.
  CHECK_OK(RunTransformPipeline(GetTransformsFromCommandLineFlags(),
                                &instruction_set));

  // Write transformed intruction set.
  const string instructions_filename =
      StrCat(FLAGS_cpu_instructions_output_file_base, "_transformed.pbtxt");
  LOG(INFO) << "Saving instruction database as: " << instructions_filename;
  x86::pdf::WriteTextProtoOrDie(instructions_filename, instruction_set);
}

}  // namespace
}  // namespace cpu_instructions

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  ::cpu_instructions::Main();
  return 0;
}
