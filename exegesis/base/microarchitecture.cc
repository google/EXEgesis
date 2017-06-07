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

StatusOr<MicroArchitectureData> MicroArchitectureData::ForCpuId(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const string& cpu_model_id) {
  if (const auto* const cpu_model = CpuModel::FromCpuId(cpu_model_id)) {
    return ForCpu(std::move(architecture_proto), *cpu_model);
  }
  return util::InvalidArgumentError(
      StrCat("Unknown CPU model '", cpu_model_id, "'"));
}

StatusOr<MicroArchitectureData> MicroArchitectureData::ForCpu(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const CpuModel& cpu_model) {
  for (const InstructionSetItinerariesProto& itineraries :
       architecture_proto->per_microarchitecture_itineraries()) {
    if (cpu_model.microarchitecture().proto().id() ==
        itineraries.microarchitecture_id()) {
      // Sanity checks.
      CHECK_EQ(itineraries.itineraries_size(),
               architecture_proto->instruction_set().instructions_size());
      return MicroArchitectureData(std::move(architecture_proto), &cpu_model,
                                   &itineraries);
    }
  }

  return util::InvalidArgumentError(
      StrCat("No itineraries for microarchitecture '",
             cpu_model.microarchitecture().proto().id(), "'"));
}

MicroArchitectureData::MicroArchitectureData(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const CpuModel* const cpu_model,
    const InstructionSetItinerariesProto* const itineraries)
    : architecture_proto_(std::move(architecture_proto)),
      cpu_model_(CHECK_NOTNULL(cpu_model)),
      itineraries_(CHECK_NOTNULL(itineraries)) {}

}  // namespace exegesis
