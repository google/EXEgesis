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

#include "exegesis/base/microarchitecture.h"

#include <utility>

#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

StatusOr<MicroArchitectureData> MicroArchitectureData::ForCpuModelId(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const string& cpu_model_id) {
  const auto* const microarchitecture =
      MicroArchitecture::FromCpuModelId(cpu_model_id);
  if (microarchitecture == nullptr) {
    return util::InvalidArgumentError(
        StrCat("Unknown CPU model '", cpu_model_id, "'"));
  }
  for (const InstructionSetItinerariesProto& itineraries :
       architecture_proto->per_microarchitecture_itineraries()) {
    if (microarchitecture->proto().id() == itineraries.microarchitecture_id()) {
      // Sanity checks.
      CHECK_EQ(itineraries.itineraries_size(),
               architecture_proto->instruction_set().instructions_size());
      return MicroArchitectureData(std::move(architecture_proto),
                                   microarchitecture, &itineraries);
    }
  }

  return util::InvalidArgumentError(
      StrCat("No itineraries for microarchitecture '",
             microarchitecture->proto().id(), "'"));
}

MicroArchitectureData::MicroArchitectureData(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const MicroArchitecture* const microarchitecture,
    const InstructionSetItinerariesProto* const itineraries)
    : architecture_proto_(std::move(architecture_proto)),
      microarchitecture_(CHECK_NOTNULL(microarchitecture)),
      itineraries_(CHECK_NOTNULL(itineraries)) {}

}  // namespace exegesis
