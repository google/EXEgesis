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

#ifndef EXEGESIS_BASE_CPUID_H_
#define EXEGESIS_BASE_CPUID_H_

#include "exegesis/base/cpu_info.h"
#include "exegesis/proto/cpuid.pb.h"

namespace exegesis {

// A "generic" version of conversion from CpuIdDumpProto to CpuInfo. Calls the
// platform-specific conversion function based on the contents of the proto.
CpuInfo CpuInfoFromCpuIdDump(const CpuIdDumpProto& cpuid_dump_proto);

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CPUID_H_
