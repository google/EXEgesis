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

// A library for working with CPUID data of x86-64 CPUs. Provides methods for
// reading the data from the host CPU, and for importing/exporting them from/to
// a text format.

#ifndef EXEGESIS_EXEGESIS_BASE_CPUID_X86_H_
#define EXEGESIS_EXEGESIS_BASE_CPUID_X86_H_

#include <cstdint>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

#include "exegesis/base/cpu_info.h"
#include "exegesis/proto/cpuid.pb.h"
#include "exegesis/proto/x86/cpuid.pb.h"
#include "util/gtl/map_util.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::StatusOr;

// A wrapper around the CPUID dump proto for x86-64. Provides methods for
// reading the dump of the host CPU, importing it from a text format, and for
// inspecting the contents of the dump.
class CpuIdDump {
 public:
  // Possible values of the vendor string used in leaf 0.
  static constexpr char kVendorStringAMD[] = "AuthenticAMD";
  static constexpr char kVendorStringIntel[] = "GenuineIntel";

  // Returns the CPUID dump for the CPU that runs the code. Returns an empty
  // CPUID dump if ran on platforms other than x86-64.
  static CpuIdDump FromHost();

  // Parses the string representation of the CPUID dump. The constructor looks
  // for lines in the format:
  //   CPUID {leaf}: {eax}-{ebx}-{ecx}-{edx}
  // where {leaf}, {eax}, {ebx}, {ecx}, and {edx} are all hexadecimal numbers
  // without the 0x prefix. The line may contain additional text separated by a
  // space; such text, and all lines not matching the format are ignored by the
  // parser.
  // Returns INVALID_ARGUMENT when the parsed dump is not valid (in the sense of
  // CpuIdDump::IsValid).
  static StatusOr<CpuIdDump> FromString(const std::string& source);

  // The default constructor creates an empty/invalid CPUID dump.
  CpuIdDump() {}
  CpuIdDump(const CpuIdDump&) = default;

  // Initializes the dump from a list of entries.
  explicit CpuIdDump(const CpuIdDumpProto& proto) : dump_proto_(proto) {}

  // Returns true if the CPUID dump is valid, i.e. it contains at least the main
  // entry (leaf = 0).
  bool IsValid() const { return GetEntryOrNull(0, 0) != nullptr; }

  // Returns the processor brand string extracted from subleafs
  // 80000002 - 80000004.
  std::string GetProcessorBrandString() const;

  // Returns the vendor string extracted from the main leaf.
  std::string GetVendorString() const;

  // Returns the CpuInfo structure corresponding to the CPU information in the
  // CPUID dump. Terminates the process with a CHECK failure when called on an
  // invalid CpuIdDump.
  CpuInfo ToCpuInfo() const;

  // Returns a string representation of the CPUID dump, in the format accepted
  // by CpuIdDump::FromString().
  std::string ToString() const;

  // Returns the entry for the given leaf and, optionally, subleaf, or nullptr
  // if the dump does not contain this entry.
  const CpuIdOutputProto* GetEntryOrNull(uint32_t leaf,
                                         uint32_t subleaf = 0) const;

  // Returns the underlying proto.
  const CpuIdDumpProto& dump_proto() const { return dump_proto_; }

 private:
  uint32_t GetNextSubleaf(uint32_t leaf) const;

  CpuIdDumpProto dump_proto_;
};

// Creates a CpuIdEntryProto with the given values.
CpuIdEntryProto MakeCpuIdEntry(uint32_t leaf, uint32_t subleaf, uint32_t eax,
                               uint32_t ebx, uint32_t ecx, uint32_t edx);

}  // namespace x86
}  // namespace exegesis

#endif  // EXEGESIS_EXEGESIS_BASE_CPUID_X86_H_
