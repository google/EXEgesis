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

#include "exegesis/base/cpuid_x86.h"

#include <cpuid.h>

#include "base/stringprintf.h"
#include "exegesis/proto/x86/cpuid.pb.h"
#include "exegesis/util/structured_register.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/str_split.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::StatusOr;

CpuIdEntryProto MakeCpuIdEntry(uint32_t leaf, uint32_t subleaf, uint32_t eax,
                               uint32_t ebx, uint32_t ecx, uint32_t edx) {
  CpuIdEntryProto entry;
  CpuIdInputProto* const input = entry.mutable_input();
  input->set_leaf(leaf);
  input->set_subleaf(subleaf);
  CpuIdOutputProto* const output = entry.mutable_output();
  output->set_eax(eax);
  output->set_ebx(ebx);
  output->set_ecx(ecx);
  output->set_edx(edx);
  return entry;
}

namespace {

// Represents the structure of registers when fetching features (EAX = 1).
struct FeatureRegisters {
  class EAXStructure : public StructuredRegister<uint32_t> {
   public:
    int step() const { return ValueAt<3, 0>(); }
    int model() const { return ValueAt<7, 4>(); }
    int family() const { return ValueAt<11, 8>(); }
    int type() const { return ValueAt<13, 12>(); }
    // 15-14 reserved.
    int emodel() const { return ValueAt<19, 16>(); }
    int efamily() const { return ValueAt<27, 20>(); }
    // 31 - 28 reserved.
  };

  class ECXStructure : public StructuredRegister<uint32_t> {
   public:
    int sse3() const { return ValueAt<0, 0>(); }
    int pclmulqdq() const { return ValueAt<1, 1>(); }
    int dtes64() const { return ValueAt<2, 2>(); }
    int monitor() const { return ValueAt<3, 3>(); }
    int dscpl() const { return ValueAt<4, 4>(); }
    int vmx() const { return ValueAt<5, 5>(); }
    int smx() const { return ValueAt<6, 6>(); }
    int est() const { return ValueAt<7, 7>(); }
    int tm2() const { return ValueAt<8, 8>(); }
    int ssse3() const { return ValueAt<9, 9>(); }
    int cntxid() const { return ValueAt<10, 10>(); }
    int sdbg() const { return ValueAt<11, 11>(); }
    int fma() const { return ValueAt<12, 12>(); }
    int cx16() const { return ValueAt<13, 13>(); }
    int xtpr() const { return ValueAt<14, 14>(); }
    int pdcm() const { return ValueAt<15, 15>(); }
    // 16 reserved.
    int pcid() const { return ValueAt<17, 17>(); }
    int dca() const { return ValueAt<18, 18>(); }
    int sse4_1() const { return ValueAt<19, 19>(); }
    int sse4_2() const { return ValueAt<20, 20>(); }
    int x2apic() const { return ValueAt<21, 21>(); }
    int movbe() const { return ValueAt<22, 22>(); }
    int popcnt() const { return ValueAt<23, 23>(); }
    int tscdadline() const { return ValueAt<24, 24>(); }
    int aes() const { return ValueAt<25, 25>(); }
    int xsave() const { return ValueAt<26, 26>(); }
    int osxsave() const { return ValueAt<27, 27>(); }
    int avx() const { return ValueAt<28, 28>(); }
    int f16c() const { return ValueAt<29, 29>(); }
    int rdrand() const { return ValueAt<30, 30>(); }
    int hypervisor() const { return ValueAt<31, 31>(); }
  };

  class EDXStructure : public StructuredRegister<uint32_t> {
   public:
    int fpu() const { return ValueAt<0, 0>(); }
    int vme() const { return ValueAt<1, 1>(); }
    int de() const { return ValueAt<2, 2>(); }
    int pse() const { return ValueAt<3, 3>(); }
    int tsc() const { return ValueAt<4, 4>(); }
    int msr() const { return ValueAt<5, 5>(); }
    int pae() const { return ValueAt<6, 6>(); }
    int mce() const { return ValueAt<7, 7>(); }
    int cx8() const { return ValueAt<8, 8>(); }
    int apic() const { return ValueAt<9, 9>(); }
    // 10 reserved.
    int sep() const { return ValueAt<11, 11>(); }
    int mtrr() const { return ValueAt<12, 12>(); }
    int pge() const { return ValueAt<13, 13>(); }
    int mca() const { return ValueAt<14, 14>(); }
    int cmov() const { return ValueAt<15, 15>(); }
    int pat() const { return ValueAt<16, 16>(); }
    int pse36() const { return ValueAt<17, 17>(); }
    int psn() const { return ValueAt<18, 18>(); }
    int clfsh() const { return ValueAt<19, 19>(); }
    // 20 reserved.
    int ds() const { return ValueAt<21, 21>(); }
    int acpi() const { return ValueAt<22, 22>(); }
    int mmx() const { return ValueAt<23, 23>(); }
    int fxsr() const { return ValueAt<24, 24>(); }
    int sse() const { return ValueAt<25, 25>(); }
    int sse2() const { return ValueAt<26, 26>(); }
    int ss() const { return ValueAt<27, 27>(); }
    int htt() const { return ValueAt<28, 28>(); }
    int tm() const { return ValueAt<29, 29>(); }
    int ia64() const { return ValueAt<30, 30>(); }
    int pbe() const { return ValueAt<31, 31>(); }
  };

  void SetValue(const CpuIdOutputProto& cpuid_out) {
    *eax.mutable_raw_value() = cpuid_out.eax();
    *ecx.mutable_raw_value() = cpuid_out.ecx();
    *edx.mutable_raw_value() = cpuid_out.edx();
  }

  EAXStructure eax;
  ECXStructure ecx;
  EDXStructure edx;
};

// Represents the structure of registers when fetching extended features (EAX =
// 7).
struct ExtendedFeatureRegisters {
  class EBXStructure : public StructuredRegister<uint32_t> {
   public:
    int fsgsbase() const { return ValueAt<0, 0>(); }
    int ia32tscadjust() const { return ValueAt<1, 1>(); }
    int sgx() const { return ValueAt<2, 2>(); }
    int bmi1() const { return ValueAt<3, 3>(); }
    int hle() const { return ValueAt<4, 4>(); }
    int avx2() const { return ValueAt<5, 5>(); }
    int reserved1() const { return ValueAt<6, 6>(); }
    int smep() const { return ValueAt<7, 7>(); }
    int bmi2() const { return ValueAt<8, 8>(); }
    int erms() const { return ValueAt<9, 9>(); }
    int invpcid() const { return ValueAt<10, 10>(); }
    int rtm() const { return ValueAt<11, 11>(); }
    int pqm() const { return ValueAt<12, 12>(); }
    int fpucsdsdeprecated() const { return ValueAt<13, 13>(); }
    int mpx() const { return ValueAt<14, 14>(); }
    int pqe() const { return ValueAt<15, 15>(); }
    int avx512f() const { return ValueAt<16, 16>(); }
    int avx512dq() const { return ValueAt<17, 17>(); }
    int rdseed() const { return ValueAt<18, 18>(); }
    int adx() const { return ValueAt<19, 19>(); }
    int smap() const { return ValueAt<20, 20>(); }
    int avx512ifma() const { return ValueAt<21, 21>(); }
    int pcommit() const { return ValueAt<22, 22>(); }
    int clflushopt() const { return ValueAt<23, 23>(); }
    int clwb() const { return ValueAt<24, 24>(); }
    int intelproctrace() const { return ValueAt<25, 25>(); }
    int avx512pf() const { return ValueAt<26, 26>(); }
    int avx512er() const { return ValueAt<27, 27>(); }
    int avx512cd() const { return ValueAt<28, 28>(); }
    int sha() const { return ValueAt<29, 29>(); }
    int avx512bw() const { return ValueAt<30, 30>(); }
    int avx512vl() const { return ValueAt<31, 31>(); }
  };

  class ECXStructure : public StructuredRegister<uint32_t> {
   public:
    int prefetchwt1() const { return ValueAt<0, 0>(); }
    // 1 reserved.
    int umip() const { return ValueAt<2, 2>(); }
    int pku() const { return ValueAt<3, 3>(); }
    int ospke() const { return ValueAt<4, 4>(); }
    // 5 - 21 reserved.
    int rdpid() const { return ValueAt<22, 22>(); }
    // 23 - 29 reserved.
    int sgx_lc() const { return ValueAt<30, 30>(); }
    int reserved4() const { return ValueAt<30, 30>(); }
  };

  void SetValue(const CpuIdOutputProto& cpuid_out) {
    *ebx.mutable_raw_value() = cpuid_out.ebx();
    *ecx.mutable_raw_value() = cpuid_out.ecx();
  }

  EBXStructure ebx;
  ECXStructure ecx;
};

// Represents the structure of registers when fetching extended features (EAX =
// 80000001H).
struct Extended2FeatureRegisters {
  class ECXStructure : public StructuredRegister<uint32_t> {
   public:
    int lahf_sahf() const { return ValueAt<0, 0>(); }
    // 1 - 4 reserved.
    int lzcnt() const { return ValueAt<5, 5>(); }
    // 6 - 7 reserved.
    int prefetchw() const { return ValueAt<8, 8>(); }
    // 9 - 31 reserved.
  };

  class EDXStructure : public StructuredRegister<uint32_t> {
   public:
    // 0 - 10 reserved.
    int syscall_sysret_64() const { return ValueAt<11, 11>(); }
    // 12 - 19 reserved.
    int execute_disable() const { return ValueAt<20, 20>(); }
    // 21 - 25 reserved.
    int gb_pages() const { return ValueAt<26, 26>(); }
    int rdtscp_ia32_tsc_aux() const { return ValueAt<27, 27>(); }
    // 28 reserved.
    int ia64() const { return ValueAt<29, 29>(); }
    // 30 - 31 reserved.
  };

  void SetValue(const CpuIdOutputProto& cpuid_out) {
    *ecx.mutable_raw_value() = cpuid_out.ecx();
    *edx.mutable_raw_value() = cpuid_out.edx();
  }

  ECXStructure ecx;
  EDXStructure edx;
};

// Represents the structure of registers when fetching extended CPU states (EAX
// = 0DH, ECX = 1).
struct ExtendedStateRegisters {
  class EAXStructure : public StructuredRegister<uint32_t> {
   public:
    int xsaveopt() const { return ValueAt<0, 0>(); }
    int xsavec() const { return ValueAt<1, 1>(); }
    int xgetbv() const { return ValueAt<2, 2>(); }
    int xsaves() const { return ValueAt<3, 3>(); }
    // 4 - 31 reserved.
  };

  void SetValue(const CpuIdOutputProto& cpuid_out) {
    *eax.mutable_raw_value() = cpuid_out.eax();
  }

  EAXStructure eax;
};

// Runs CPUID for the given leaf and subleaf. On platforms where CPUID does not
// exist returns an empty CpuIdEntryProto.
CpuIdEntryProto GetHostCpuIdDumpEntry(uint32_t leaf, uint32_t subleaf) {
  uint32_t eax = 0;
  uint32_t ebx = 0;
  uint32_t ecx = subleaf;
  uint32_t edx = 0;
#ifdef __x86_64__
  __cpuid(leaf, eax, ebx, ecx, edx);
#endif  // __x86-64__
  return MakeCpuIdEntry(leaf, subleaf, eax, ebx, ecx, edx);
}

}  // namespace

StatusOr<CpuIdDump> CpuIdDump::FromString(const string& source) {
  static const LazyRE2 regex = {
      " *CPUID +([0-9a-fA-F]+): +"
      "([0-9a-fA-F]+)-([0-9a-fA-F]+)-([0-9a-fA-F]+)-([0-9a-fA-F]+)(?: .*)?"};
  CpuIdDump dump;
  X86CpuIdDumpProto* const dump_proto =
      dump.dump_proto_.mutable_x86_cpuid_dump();
  const std::vector<string> lines = strings::Split(source, "\n");
  for (const string& line : lines) {
    uint32_t leaf = 0;
    uint32_t eax = 0;
    uint32_t ebx = 0;
    uint32_t ecx = 0;
    uint32_t edx = 0;
    if (RE2::FullMatch(line, *regex, RE2::Hex(&leaf), RE2::Hex(&eax),
                       RE2::Hex(&ebx), RE2::Hex(&ecx), RE2::Hex(&edx))) {
      const uint32_t subleaf = dump.GetNextSubleaf(leaf);
      *dump_proto->add_entries() =
          MakeCpuIdEntry(leaf, subleaf, eax, ebx, ecx, edx);
    }
  }
  if (!dump.IsValid()) {
    return InvalidArgumentError("Leaf 0 was not found in the parsed dump.");
  }
  return dump;
}

CpuIdDump CpuIdDump::FromHost() {
  CpuIdDump dump;
  X86CpuIdDumpProto* const dump_proto =
      dump.dump_proto_.mutable_x86_cpuid_dump();
#ifdef __x86_64__
  const CpuIdEntryProto root_leaf = GetHostCpuIdDumpEntry(0, 0);
  *dump_proto->add_entries() = root_leaf;
  const uint32_t num_leafs = root_leaf.output().eax();
  for (uint32_t leaf = 1; leaf < num_leafs; ++leaf) {
    // TODO(ondrasej): Handle subleafs properly.
    *dump_proto->add_entries() = GetHostCpuIdDumpEntry(leaf, 0);
  }
#endif  // __x86_64__
  return dump;
}

string CpuIdDump::GetVendorString() const {
  const CpuIdOutputProto* const root = GetEntryOrNull(0, 0);
  CHECK(root) << "Invalid CPUID dump!";
  char buffer[12];
  uint32_t* data = reinterpret_cast<uint32_t*>(buffer);
  data[0] = root->ebx();
  data[1] = root->edx();
  data[2] = root->ecx();
  return string(buffer, 12);
}

#define PROCESS_FEATURE(name, reg, field)  \
  if (reg.field()) {                       \
    InsertOrDie(&indexed_features, #name); \
  }

CpuInfo CpuIdDump::ToCpuInfo() const {
  CHECK(IsValid());
  FeatureRegisters features;
  ExtendedFeatureRegisters ext_features;
  Extended2FeatureRegisters ext2_features;
  ExtendedStateRegisters ext_state;

  for (const auto& entry : dump_proto().x86_cpuid_dump().entries()) {
    switch (entry.input().leaf()) {
      case 0x01:
        features.SetValue(entry.output());
        break;
      case 0x07:
        if (entry.input().subleaf() == 0) ext_features.SetValue(entry.output());
        break;
      case 0x0D:
        if (entry.input().subleaf() == 1) ext_state.SetValue(entry.output());
        break;
      case 0x80000001:
        ext2_features.SetValue(entry.output());
        break;
    }
  }

  std::unordered_set<string> indexed_features;

  PROCESS_FEATURE(3DNOW, ext_features.ecx, prefetchwt1);
  PROCESS_FEATURE(ADX, ext_features.ebx, adx);
  PROCESS_FEATURE(CLFLUSHOPT, ext_features.ebx, clflushopt);
  PROCESS_FEATURE(AES, features.ecx, aes);
  PROCESS_FEATURE(AVX, features.ecx, avx);
  PROCESS_FEATURE(AVX2, ext_features.ebx, avx2);
  PROCESS_FEATURE(BMI1, ext_features.ebx, bmi1);
  PROCESS_FEATURE(BMI2, ext_features.ebx, bmi2);
  PROCESS_FEATURE(CLMUL, features.ecx, pclmulqdq);
  PROCESS_FEATURE(F16C, features.ecx, f16c);
  PROCESS_FEATURE(FMA, features.ecx, fma);
  PROCESS_FEATURE(FPU, features.edx, fpu);
  PROCESS_FEATURE(CLFSH, features.edx, clfsh);
  PROCESS_FEATURE(FSGSBASE, ext_features.ebx, fsgsbase);
  PROCESS_FEATURE(HLE, ext_features.ebx, hle);
  PROCESS_FEATURE(INVPCID, ext_features.ebx, invpcid);
  PROCESS_FEATURE(LZCNT, ext2_features.ecx, lzcnt);
  PROCESS_FEATURE(MMX, features.edx, mmx);
  PROCESS_FEATURE(MOVBE, features.ecx, movbe);
  PROCESS_FEATURE(MPX, ext_features.ebx, mpx);
  PROCESS_FEATURE(OSPKE, ext_features.ecx, ospke);
  PROCESS_FEATURE(PRFCHW, ext2_features.ecx, prefetchw);
  PROCESS_FEATURE(RDPID, ext_features.ecx, rdpid);
  PROCESS_FEATURE(RDRAND, features.ecx, rdrand);
  PROCESS_FEATURE(RDSEED, ext_features.ebx, rdseed);
  PROCESS_FEATURE(RTM, ext_features.ebx, rtm);
  PROCESS_FEATURE(SHA, ext_features.ebx, sha);
  PROCESS_FEATURE(SMAP, ext_features.ebx, smap);
  PROCESS_FEATURE(SSE, features.edx, sse);
  PROCESS_FEATURE(SSE2, features.edx, sse2);
  PROCESS_FEATURE(SSE3, features.ecx, sse3);
  PROCESS_FEATURE(SSE4_1, features.ecx, sse4_1);
  PROCESS_FEATURE(SSE4_2, features.ecx, sse4_2);
  PROCESS_FEATURE(SSSE3, features.ecx, ssse3);
  PROCESS_FEATURE(XSAVEOPT, ext_state.eax, xsaveopt);

  // See CPUID doc for the algorithm.
  const int family =
      features.eax.family() != 0x0f
          ? features.eax.family()
          : (features.eax.efamily() << 4) + features.eax.family();
  const int model =
      (features.eax.family() == 0x06 || features.eax.family() == 0x0f)
          ? (features.eax.emodel() << 4) + features.eax.model()
          : features.eax.model();

  return CpuInfo(StringPrintf("intel:%02X_%02X", family, model),
                 std::move(indexed_features));
}

#undef PROCESS_FEATURE

string CpuIdDump::ToString() const {
  string buffer;
  for (const CpuIdEntryProto& entry : dump_proto_.x86_cpuid_dump().entries()) {
    if (!buffer.empty()) buffer += "\n";
    StringAppendF(&buffer, "CPUID %08X: %08X-%08X-%08X-%08X",
                  entry.input().leaf(), entry.output().eax(),
                  entry.output().ebx(), entry.output().ecx(),
                  entry.output().edx());
  }
  return buffer;
}

const CpuIdOutputProto* CpuIdDump::GetEntryOrNull(
    uint32_t leaf, uint32_t subleaf /* = 0 */) const {
  for (const CpuIdEntryProto& entry : dump_proto_.x86_cpuid_dump().entries()) {
    const CpuIdInputProto& input = entry.input();
    if (input.leaf() == leaf && input.subleaf() == subleaf) {
      return &entry.output();
    }
  }
  return nullptr;
}

uint32_t CpuIdDump::GetNextSubleaf(uint32_t leaf) const {
  CHECK_LT(leaf, 0xffffffff);
  uint32_t next_subleaf = 0;
  for (const CpuIdEntryProto& entry : dump_proto_.x86_cpuid_dump().entries()) {
    const CpuIdInputProto& input = entry.input();
    if (input.leaf() == leaf && next_subleaf <= input.subleaf()) {
      next_subleaf = input.subleaf() + 1;
    }
  }
  return next_subleaf;
}

}  // namespace x86
}  // namespace exegesis
