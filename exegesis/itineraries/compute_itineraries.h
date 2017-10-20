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

// A library to compute instruction itineraries.

#ifndef EXEGESIS_ITINERARIES_COMPUTE_ITINERARIES_H_
#define EXEGESIS_ITINERARIES_COMPUTE_ITINERARIES_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace itineraries {

using ::exegesis::util::Status;

// Computes the itinerary of every instruction.
// NOTE(bdb): Some instructions are not yet handled. For the supported
// instructions, some addressing modes are not handled.
Status ComputeItineraries(const InstructionSetProto& instruction_set,
                          InstructionSetItinerariesProto* itineraries);

}  // namespace itineraries
}  // namespace exegesis

#endif  // EXEGESIS_ITINERARIES_COMPUTE_ITINERARIES_H_
