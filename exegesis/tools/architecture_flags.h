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

#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/instructions.pb.h"
#include "gflags/gflags.h"

DECLARE_string(exegesis_architecture);

namespace exegesis {

// Returns the architecture proto for the architecture specified in the
// command-line flag --exegesis_architecture.
// Terminates the process if the specification of the architecture is not valid,
// or the architecture can't be read from the source.
ArchitectureProto GetArchitectureFromCommandLineFlagsOrDie();

// Returns the instruction set for the architecture specified in the
// command-line flag --exegesis_architecture.
// Terminates the process if the specification of the architecture is not valid,
// or the architecture can't be read from the source.
InstructionSetProto GetInstructionSetFromCommandLineFlagsOrDie();

// Returns the instruction set and itineraries for the microarchitecture
// specified in the command-line flag --exegesis_cpu_model.
MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags();

}  // namespace exegesis

#endif  // EXEGESIS_TOOLS_ARCHITECTURE_FLAGS_H_
