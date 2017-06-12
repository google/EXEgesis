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

#include "exegesis/base/cpu_model.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

class MicroArchitectureData {
 public:
  // Given an architecture and a CPU model id, retrieves the data for the given
  // model id.
  static StatusOr<MicroArchitectureData> ForCpuModelId(
      std::shared_ptr<const ArchitectureProto> architecture_proto,
      const string& cpu_model_id);

  // StatusOr<T> requires T to be default-constructible.
  // TODO(courbet): Remove when StatusOr is fixed.
  MicroArchitectureData()
      : microarchitecture_(nullptr), itineraries_(nullptr) {}

  // For tests.
  MicroArchitectureData(
      std::shared_ptr<const ArchitectureProto> architecture_proto,
      const MicroArchitecture* microarchitecture,
      const InstructionSetItinerariesProto* itineraries);

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
  // Keep a reference to the underlying data (instruction_set and itineraries
  // point into architecture_proto).
  // The following fields are never nullptr.
  std::shared_ptr<const ArchitectureProto> architecture_proto_;
  const MicroArchitecture* microarchitecture_;
  const InstructionSetItinerariesProto* itineraries_;
};

}  // namespace exegesis

#endif  // EXEGESIS_BASE_MICROARCHITECTURE_H_
