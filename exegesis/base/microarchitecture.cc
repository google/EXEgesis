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

#include <unordered_map>
#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

MicroArchitecture::MicroArchitecture(const MicroArchitectureProto& proto)
    : proto_(proto),
      port_masks_(proto_.port_masks().begin(), proto_.port_masks().end()) {}

const PortMask* MicroArchitecture::load_store_address_generation() const {
  return GetPortMaskOrNull(
      proto_.load_store_address_generation_port_mask_index() - 1);
}

const PortMask* MicroArchitecture::store_address_generation() const {
  return GetPortMaskOrNull(proto_.store_address_generation_port_mask_index() -
                           1);
}

const PortMask* MicroArchitecture::store_data() const {
  return GetPortMaskOrNull(proto_.store_data_port_mask_index() - 1);
}

const PortMask* MicroArchitecture::GetPortMaskOrNull(const int index) const {
  return index < 0 ? nullptr : &port_masks_[index];
}

bool MicroArchitecture::IsProtectedMode(int protection_mode) const {
  CHECK_NE(proto_.protected_mode().protected_modes().empty(),
           proto_.protected_mode().user_modes().empty());
  if (proto_.protected_mode().protected_modes().empty()) {
    for (int mode : proto_.protected_mode().user_modes()) {
      if (protection_mode == mode) {
        return false;
      }
    }
    return true;
  } else {
    for (int mode : proto_.protected_mode().protected_modes()) {
      if (protection_mode == mode) {
        return true;
      }
    }
    return false;
  }
}

namespace {

std::unordered_map<std::string, std::unique_ptr<const MicroArchitecture>>*
MicroArchitecturesById() {
  static auto* const result =
      new std::unordered_map<std::string,
                             std::unique_ptr<const MicroArchitecture>>();
  return result;
}

std::unordered_map<std::string, const MicroArchitecture*>*
MicroArchitecturesByCpuModelId() {
  static auto* const result =
      new std::unordered_map<std::string, const MicroArchitecture*>();
  return result;
}

}  // namespace

namespace internal {

void RegisterMicroArchitectures::RegisterFromProto(
    const MicroArchitecturesProto& microarchitectures) {
  auto* const microarchitectures_by_id = MicroArchitecturesById();
  auto* const microarchitectures_by_cpu_model_id =
      MicroArchitecturesByCpuModelId();
  for (const MicroArchitectureProto& microarchitecture_proto :
       microarchitectures.microarchitectures()) {
    auto microarchitecture =
        gtl::MakeUnique<MicroArchitecture>(microarchitecture_proto);
    for (const std::string& model_id : microarchitecture_proto.model_ids()) {
      InsertOrDie(microarchitectures_by_cpu_model_id, model_id,
                  microarchitecture.get());
    }
    const std::string& microarchitecture_id = microarchitecture_proto.id();
    const auto insert_result = microarchitectures_by_id->emplace(
        microarchitecture_id, std::move(microarchitecture));
    if (!insert_result.second) {
      LOG(FATAL) << "Duplicate micro-architecture: " << microarchitecture_id;
    }
  }
}

}  // namespace internal

const MicroArchitecture* MicroArchitecture::FromId(
    const std::string& microarchitecture_id) {
  const std::unique_ptr<const MicroArchitecture>* const result =
      FindOrNull(*MicroArchitecturesById(), microarchitecture_id);
  return result ? result->get() : nullptr;
}

const MicroArchitecture& MicroArchitecture::FromIdOrDie(
    const std::string& microarchitecture_id) {
  return *CHECK_NOTNULL(FromId(microarchitecture_id));
}

const MicroArchitecture* MicroArchitecture::FromCpuModelId(
    const std::string& cpu_model_id) {
  const auto* const result =
      FindPtrOrNull(*MicroArchitecturesByCpuModelId(), cpu_model_id);
  if (result == nullptr) {
    LOG(WARNING) << "Unknown CPU model '" << cpu_model_id << "'";
  }
  return result;
}

const MicroArchitecture& MicroArchitecture::FromCpuModelIdOrDie(
    const std::string& cpu_model_id) {
  return *CHECK_NOTNULL(FromCpuModelId(cpu_model_id));
}

StatusOr<MicroArchitectureData> MicroArchitectureData::ForCpuModelId(
    std::shared_ptr<const ArchitectureProto> architecture_proto,
    const std::string& cpu_model_id) {
  const auto* const microarchitecture =
      MicroArchitecture::FromCpuModelId(cpu_model_id);
  if (microarchitecture == nullptr) {
    return util::InvalidArgumentError(
        absl::StrCat("Unknown CPU model '", cpu_model_id, "'"));
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
      absl::StrCat("No itineraries for microarchitecture '",
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
