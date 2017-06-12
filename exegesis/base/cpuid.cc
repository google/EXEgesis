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

#include "exegesis/base/cpuid.h"

#include "exegesis/base/cpuid_x86.h"
#include "glog/logging.h"

namespace exegesis {

CpuInfo CpuInfoFromCpuIdDump(const CpuIdDumpProto& cpuid_dump_proto) {
  switch (cpuid_dump_proto.dump_case()) {
    case CpuIdDumpProto::kX86CpuidDump: {
      x86::CpuIdDump dump(cpuid_dump_proto);
      CHECK(dump.IsValid());
      return dump.ToCpuInfo();
    } break;
    case CpuIdDumpProto::DUMP_NOT_SET:
      break;
  }
  return CpuInfo(CpuInfoProto());
}

}  // namespace exegesis
