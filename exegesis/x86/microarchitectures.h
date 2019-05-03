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

#ifndef EXEGESIS_X86_MICROARCHITECTURES_H_
#define EXEGESIS_X86_MICROARCHITECTURES_H_

#include "exegesis/base/microarchitecture.h"

namespace exegesis {
namespace x86 {

// For tests only. These functions return a CPU model that belongs to the
// microarchitecture in the name of the function. They are sorted in reverse
// chronological order.
extern const char* const kExampleSkylakeCpuModelId;
extern const char* const kExampleBroadwellCpuModelId;
extern const char* const kExampleHaswellCpuModelId;
extern const char* const kExampleIvyBridgeCpuModelId;
extern const char* const kExampleSandyBridgeCpuModelId;
extern const char* const kExampleWestmereCpuModelId;
extern const char* const kExampleNehalemCpuModelId;

// The microarchitectures in reverse chronological order.
inline const MicroArchitecture& SkylakeMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("skl");
}

inline const MicroArchitecture& BroadwellMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("bdw");
}

inline const MicroArchitecture& HaswellMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("hsw");
}

inline const MicroArchitecture& IvyBridgeMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("snb");
}

inline const MicroArchitecture& SandyBridgeMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("snb");
}

inline const MicroArchitecture& WestmereMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("wsm");
}

inline const MicroArchitecture& NehalemMicroArchitecture() {
  return MicroArchitecture::FromIdOrDie("nhm");
}

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_X86_MICROARCHITECTURES_H_
