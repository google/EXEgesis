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

#ifndef CPU_INSTRUCTIONS_X86_MICROARCHITECTURES_H_
#define CPU_INSTRUCTIONS_X86_MICROARCHITECTURES_H_

#include "cpu_instructions/base/cpu_model.h"

namespace cpu_instructions {
namespace x86 {

// For tests only. These functions return a CPU model that belongs to the
// microarchitecture in the name of the function. They are sorted in reverse
// chronological order.
inline const CpuModel& SkylakeCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_4E");
}

inline const CpuModel& BroadwellCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_3D");
}

inline const CpuModel& HaswellCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_3C");
}

inline const CpuModel& IvyBridgeCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_3A");
}

inline const CpuModel& SandyBridgeCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_2A");
}

inline const CpuModel& WestmereCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_25");
}

inline const CpuModel& NehalemCpuModel() {
  return CpuModel::FromCpuIdOrDie("intel:06_1A");
}

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
}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_X86_MICROARCHITECTURES_H_
