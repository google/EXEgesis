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

#ifndef EXEGESIS_BASE_HOST_CPU_H_
#define EXEGESIS_BASE_HOST_CPU_H_

#include "exegesis/base/cpu_info.h"
#include "exegesis/proto/cpuid.pb.h"

namespace exegesis {

// Returns the CPUID dump of the host CPU. Note that the function caches the
// results, and only uses the CPUID instruction or its equivalent on the
// first call. It causes a CHECK failure on platforms where CPUID is not
// supported.
const CpuIdDumpProto& HostCpuIdDumpOrDie();

// Returns the CpuInfo structure for the host CPU. Note that the function caches
// the results, and only uses the CPUID instruction or its equivalent on
// the first call. It causes a CHECK failure on platforms where the information
// could not be obtained.
const CpuInfo& HostCpuInfoOrDie();

}  // namespace exegesis

#endif  // EXEGESIS_BASE_HOST_CPU_H_
