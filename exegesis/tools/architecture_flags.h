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

// Provides access to instruction sets for all supported architectures.

#ifndef EXEGESIS_TOOLS_ARCHITECTURE_FLAGS_H_
#define EXEGESIS_TOOLS_ARCHITECTURE_FLAGS_H_

#include <memory>
#include <string>

#include "absl/flags/declare.h"
#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/instructions.pb.h"

ABSL_DECLARE_FLAG(std::string, exegesis_architecture);
ABSL_DECLARE_FLAG(std::string, exegesis_microarchitecture);

namespace exegesis {

// Checks the flag --exegesis_list_architectures. If it is set, prints the list
// of registered architectures to STDOUT and terminates the process with exit
// code 0. Otherwise, does nothing.
void ListRegisteredArchitecturesAndExitIfRequested();

// Returns the architecture proto for the architecture specified in the
// command-line flag --exegesis_architecture.
// Terminates the process if the specification of the architecture is not valid,
// or the architecture can't be read from the source.
std::shared_ptr<const ArchitectureProto>
GetArchitectureFromCommandLineFlagsOrDie();

// Returns the instruction set and itineraries for the microarchitecture
// specified in the command-line flag --exegesis_cpu_model.
MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags();

}  // namespace exegesis

#endif  // EXEGESIS_TOOLS_ARCHITECTURE_FLAGS_H_
