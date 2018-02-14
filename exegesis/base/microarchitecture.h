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

// Represents all data about a microarchitecture: instructions and their
// itineraries, as well as the microarchitecture itself.
#ifndef EXEGESIS_BASE_MICROARCHITECTURE_H_
#define EXEGESIS_BASE_MICROARCHITECTURE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "exegesis/base/port_mask.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Returns the microarchitecture id for a CPU model id.
StatusOr<std::string> GetMicroArchitectureForCpuModelId(
    const std::string& cpu_model_id);
const std::string& GetMicroArchitectureIdForCpuModelOrDie(
    const std::string& cpu_model_id);

// Represents a MicroArchitectureProto in memory. See the proto documentation
// for details.
class MicroArchitecture {
 public:
  // Returns nullptr if unknown.
  static const MicroArchitecture* FromId(
      const std::string& microarchitecture_id);
  // Dies if unknown.
  static const MicroArchitecture& FromIdOrDie(
      const std::string& microarchitecture_id);
  static const MicroArchitecture& FromCpuModelIdOrDie(
      const std::string& cpu_model_id);

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

 private:
  MicroArchitecture() = delete;
  MicroArchitecture(const MicroArchitecture&) = delete;

  const PortMask* GetPortMaskOrNull(const int index) const;

  const MicroArchitectureProto proto_;
  const std::vector<PortMask> port_masks_;
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

class MicroArchitectureData {
 public:
  // Creates a MicroArchitectureData pack from an ArchitectureProto and a
  // microarchitecture_id.
  static StatusOr<MicroArchitectureData> ForMicroArchitectureId(
      std::shared_ptr<const ArchitectureProto> architecture_proto,
      const std::string& microarchitecture_id);

  // TODO(ondrasej): Remove this method when the microarchitecture data is
  // merged with the ArchitectureProto.
  static StatusOr<MicroArchitectureData> ForMicroArchitecture(
      std::shared_ptr<const ArchitectureProto> architecture_proto,
      const MicroArchitecture* microarchitecture);

  // StatusOr<T> requires T to be default-constructible.
  // TODO(courbet): Remove when StatusOr is fixed.
  MicroArchitectureData()
      : microarchitecture_(nullptr), itineraries_(nullptr) {}

  std::shared_ptr<const ArchitectureProto> architecture() const {
    return architecture_proto_;
  }

  const InstructionSetProto& instruction_set() const {
    return architecture_proto_->instruction_set();
  }

  const InstructionSetItinerariesProto& itineraries() const {
    return *itineraries_;
  }

  const MicroArchitecture& microarchitecture() const {
    return *microarchitecture_;
  }

 private:
  MicroArchitectureData(
      std::shared_ptr<const ArchitectureProto> architecture_proto,
      const MicroArchitecture* microarchitecture,
      const InstructionSetItinerariesProto* itineraries);

  // Keep a reference to the underlying data (instruction_set and itineraries
  // point into architecture_proto).
  // The following fields are never nullptr.
  std::shared_ptr<const ArchitectureProto> architecture_proto_;
  const MicroArchitecture* microarchitecture_;
  const InstructionSetItinerariesProto* itineraries_;
};

}  // namespace exegesis

#endif  // EXEGESIS_BASE_MICROARCHITECTURE_H_
