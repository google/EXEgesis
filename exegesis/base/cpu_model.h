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

#ifndef EXEGESIS_BASE_CPU_MODEL_H_
#define EXEGESIS_BASE_CPU_MODEL_H_

#include <vector>
#include "strings/string.h"

#include "exegesis/base/port_mask.h"
#include "exegesis/proto/microarchitecture.pb.h"

namespace exegesis {

class MicroArchitecture;

// Represents a CpuModelProto in memory. See the proto documentation for
// details.
class CpuModel {
 public:
  // Returns nullptr if the CPU model is unknown.
  static const CpuModel* FromCpuId(const string& cpu_id);
  // Dies when the CPU model is unknown.
  static const CpuModel& FromCpuIdOrDie(const string& cpu_id);

  CpuModel(const CpuModelProto* proto,
           const MicroArchitecture* microarchitecture);
  CpuModel(CpuModel&&) = default;

  const CpuModelProto& proto() const { return *proto_; }

  const MicroArchitecture& microarchitecture() const;

 private:
  CpuModel() = delete;
  CpuModel(const CpuModel&) = delete;

  const CpuModelProto* const proto_;
  const MicroArchitecture* const microarchitecture_;
};

// Represents a MicroArchitectureProto in memory. See the proto documentation
// for details.
class MicroArchitecture {
 public:
  // Returns nullptr if unknown.
  static const MicroArchitecture* FromId(const string& microarchitecture_id);
  // Dies if unknown.
  static const MicroArchitecture& FromIdOrDie(
      const string& microarchitecture_id);

  explicit MicroArchitecture(const MicroArchitectureProto& proto);

  const MicroArchitectureProto& proto() const { return proto_; }

  // Returns the port masks, in the same order as the proto.
  const std::vector<PortMask>& port_masks() const { return port_masks_; }

  // Port masks with specific semantics. Returns null if unavailable.
  const PortMask* load_store_address_generation() const;
  const PortMask* store_address_generation() const;
  const PortMask* store_data() const;

  // Returns true if a protection mode is in the protected range (e.g. 0 is
  // protected in x86 but 3 is not). protection_mode < 0 is the default.
  bool IsProtectedMode(int protection_mode) const;

  const std::vector<CpuModel>& cpu_models() const { return cpu_models_; }

 private:
  MicroArchitecture() = delete;
  MicroArchitecture(const MicroArchitecture&) = delete;

  const PortMask* GetPortMaskOrNull(const int index) const;

  const MicroArchitectureProto proto_;
  const std::vector<PortMask> port_masks_;
  std::vector<CpuModel> cpu_models_;
};

// Registers a list of micro-architectures to make them and their CPU models
// available through MicroArchitecture::FromId, and CpuModel::FromId. The macro
// takes a single parameter 'provider'. This must be a callable object (e.g.
// a function pointer, a functor, a std::function object) that returns an object
// convertible to const MicroArchitecturesProto&.
#define REGISTER_MICRO_ARCHITECTURES(provider)     \
  ::exegesis::internal::RegisterMicroArchitectures \
      register_micro_architectures_##provider(provider);

namespace internal {

// A helper class used for the implementation of the registerer; the constructor
// registers the microarchitectures returned by the provider.
class RegisterMicroArchitectures {
 public:
  template <typename Provider>
  RegisterMicroArchitectures(Provider provider) {
    RegisterFromProto(provider());
  }

 private:
  static void RegisterFromProto(const MicroArchitecturesProto& proto);
};

}  // namespace internal

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CPU_MODEL_H_
