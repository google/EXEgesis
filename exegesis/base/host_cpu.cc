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

#include "exegesis/base/host_cpu.h"

#include "exegesis/base/cpuid.h"
#include "exegesis/base/cpuid_x86.h"
#include "glog/logging.h"

namespace exegesis {
namespace {

// Reads the host CPUID dump (or equaivalent).
CpuIdDumpProto GetHostCpuIdDumpOrDie() {
#ifdef __x86_64__
  const x86::CpuIdDump dump = x86::CpuIdDump::FromHost();
  CHECK(dump.IsValid());
  return dump.dump_proto();
#else
// TODO(courbet, ondrasej): Add support for ARM and POWER if needed. The code
// should also work on 32-bit x86, but we do not support 32-bit.
#error "CPUID or equivalent is not supported on this architecture."
#endif  // __x86_64__
}

}  // namespace

const CpuIdDumpProto& HostCpuIdDumpOrDie() {
  static const CpuIdDumpProto* const dump_proto =
      new CpuIdDumpProto(GetHostCpuIdDumpOrDie());
  return *dump_proto;
}

const CpuInfo& HostCpuInfoOrDie() {
  static const CpuInfo* const cpu_info =
      new CpuInfo(CpuInfoFromCpuIdDump(HostCpuIdDumpOrDie()));
  return *cpu_info;
}

}  // namespace exegesis
