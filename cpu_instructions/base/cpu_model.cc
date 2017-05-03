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

#include "cpu_instructions/base/cpu_model.h"

#include <unordered_map>

#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"

namespace cpu_instructions {

MicroArchitecture::MicroArchitecture(const MicroArchitectureProto& proto)
    : proto_(proto),
      port_masks_(proto_.port_masks().begin(), proto_.port_masks().end()) {
  for (const CpuModelProto& model_proto : proto_.cpu_models()) {
    cpu_models_.push_back(CpuModel(&model_proto, this));
  }
}

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

using MicroArchitectureRegistry =
    std::unordered_map<string, std::unique_ptr<MicroArchitecture>>;

MicroArchitectureRegistry* KnownMicroArchitectures() {
  static auto* const registry = new MicroArchitectureRegistry();
  return registry;
}

using CpuModelRegistry = std::unordered_map<string, const CpuModel*>;

CpuModelRegistry* KnownCpuModels() {
  static auto* const registry = new CpuModelRegistry();
  return registry;
}

}  // namespace

namespace internal {

void RegisterMicroArchitectures::RegisterFromProto(
    const MicroArchitecturesProto& microarchitectures) {
  MicroArchitectureRegistry* const microarchitecture_registry =
      KnownMicroArchitectures();
  CpuModelRegistry* const cpu_registry = KnownCpuModels();
  for (const MicroArchitectureProto& microarchitecture_proto :
       microarchitectures.microarchitectures()) {
    auto microarchitecture =
        gtl::MakeUnique<MicroArchitecture>(microarchitecture_proto);
    for (const CpuModel& model : microarchitecture->cpu_models()) {
      InsertOrDie(cpu_registry, model.proto().id(), &model);
    }
    const string& microarchitecture_id = microarchitecture_proto.id();
    const auto insert_result = microarchitecture_registry->emplace(
        microarchitecture_id, std::move(microarchitecture));
    if (!insert_result.second) {
      LOG(FATAL) << "Duplicate micro-architecture: " << microarchitecture_id;
    }
  }
}

}  // namespace internal

const MicroArchitecture* MicroArchitecture::FromId(
    const string& microarchitecture_id) {
  const std::unique_ptr<MicroArchitecture>* const result =
      FindOrNull(*KnownMicroArchitectures(), microarchitecture_id);
  return result ? result->get() : nullptr;
}

const MicroArchitecture& MicroArchitecture::FromIdOrDie(
    const string& microarchitecture_id) {
  return *CHECK_NOTNULL(FromId(microarchitecture_id));
}

CpuModel::CpuModel(const CpuModelProto* proto,
                   const MicroArchitecture* microarchitecture)
    : proto_(CHECK_NOTNULL(proto)),
      microarchitecture_(CHECK_NOTNULL(microarchitecture)) {}

const MicroArchitecture& CpuModel::microarchitecture() const {
  return *microarchitecture_;
}

namespace {}  // namespace

const CpuModel* CpuModel::FromCpuId(const string& cpu_id) {
  const auto* const result = FindPtrOrNull(*KnownCpuModels(), cpu_id);
  if (result == nullptr) {
    LOG(WARNING) << "Unknown CPU with id '" << cpu_id << "'";
  }
  return result;
}

const CpuModel& CpuModel::FromCpuIdOrDie(const string& cpu_id) {
  return *CHECK_NOTNULL(FromCpuId(cpu_id));
}

}  // namespace cpu_instructions
