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

#include "exegesis/tools/architecture_flags.h"

#include "exegesis/base/architecture_provider.h"
#include "exegesis/base/microarchitecture.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

DEFINE_string(exegesis_architecture, "",
              "The name of the architecture for which the code is optimized."
              "If 'intel', then the raw parsed output (stright out of SDM) is"
              "returned."
              "If this is not one of the known sources, we'll try to interpret "
              "this as a file.");

DEFINE_string(exegesis_cpu_model, "intel:06_3F",
              "The id of the CPU model for which the code is optimized.");

namespace exegesis {

void CheckArchitectureFlag() {
  CHECK(!FLAGS_exegesis_architecture.empty())
      << "Please provide an architecture (e.g. 'pbtxt:/path/to/file.pb.txt')";
}

ArchitectureProto GetArchitectureFromCommandLineFlagsOrDie() {
  CheckArchitectureFlag();
  return *GetArchitectureProtoOrDie(FLAGS_exegesis_architecture);
}

InstructionSetProto GetInstructionSetFromCommandLineFlagsOrDie() {
  CheckArchitectureFlag();
  return GetArchitectureProtoOrDie(FLAGS_exegesis_architecture)
      ->instruction_set();
}

MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags() {
  CheckArchitectureFlag();
  return MicroArchitectureData::ForCpuModelId(
             GetArchitectureProtoOrDie(FLAGS_exegesis_architecture),
             FLAGS_exegesis_cpu_model)
      .ValueOrDie();
}

}  // namespace exegesis
