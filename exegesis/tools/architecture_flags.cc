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

#include <cstdlib>
#include <string>

#include "absl/flags/flag.h"
#include "exegesis/base/architecture_provider.h"
#include "exegesis/base/microarchitecture.h"
#include "glog/logging.h"

ABSL_FLAG(bool, exegesis_list_architectures, false,
          "Print the list of registered architectures.");
ABSL_FLAG(std::string, exegesis_architecture, "",
          "The name of the architecture for which the code is optimized."
          "If 'intel', then the raw parsed output (stright out of SDM) is"
          "returned."
          "If this is not one of the known sources, we'll try to interpret "
          "this as a file.");

ABSL_FLAG(std::string, exegesis_microarchitecture, "hsw",
          "The id of the microarchitecture for which the code is optimized.");

namespace exegesis {

void ListRegisteredArchitecturesAndExitIfRequested() {
  if (absl::GetFlag(FLAGS_exegesis_list_architectures)) {
    const std::vector<std::string> registered_architectures =
        GetRegisteredArchitectureIds();
    printf("Registered architectures:\n");
    for (const std::string& architecture_id : registered_architectures) {
      printf("  %s\n", architecture_id.c_str());
    }
    exit(0);
  }
}

void CheckArchitectureFlag() {
  CHECK(!absl::GetFlag(FLAGS_exegesis_architecture).empty())
      << "Please provide an architecture (e.g. 'pbtxt:/path/to/file.pb.txt')";
}

std::shared_ptr<const ArchitectureProto>
GetArchitectureFromCommandLineFlagsOrDie() {
  CheckArchitectureFlag();
  return GetArchitectureProtoOrDie(absl::GetFlag(FLAGS_exegesis_architecture));
}

MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags() {
  CheckArchitectureFlag();
  return MicroArchitectureData::ForMicroArchitectureId(
             GetArchitectureProtoOrDie(
                 absl::GetFlag(FLAGS_exegesis_architecture)),
             absl::GetFlag(FLAGS_exegesis_microarchitecture))
      .ValueOrDie();
}

}  // namespace exegesis
