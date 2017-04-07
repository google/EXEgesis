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

#include "cpu_instructions/base/host_cpu.h"

#include <cpuid.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>

#include "glog/logging.h"

#include "base/stringprintf.h"
#include "strings/str_cat.h"
#include "strings/str_split.h"
#include "util/gtl/map_util.h"

namespace cpu_instructions {
namespace {

#ifdef __x86_64__

// Represents a register with access to individual bit ranges.
class StructuredRegister {
 public:
  uint32_t* mutable_raw_value() { return &raw_value_; }
  uint32_t raw_value() const { return raw_value_; }

  // Returns the bit range [msb, lsb] as an integer.
  template <int msb, int lsb>
  int ValueAt() const {
    static_assert(msb >= 0, "msb must be >= 0");
    static_assert(msb < 32, "msb must be < 32");
    static_assert(lsb >= 0, "lsb must be >= 0");
    static_assert(lsb < 32, "lsb must be < 32");
    static_assert(lsb <= msb, "lsb must be <= msb");
    return (raw_value_ >> lsb) &
           static_cast<uint32_t>((1ull << (msb - lsb + 1)) - 1ull);
  }

 private:
  uint32_t raw_value_ = 0;
};

// Represents the structure of registers when fetching features (EAX = 1).
struct FeatureRegisters {
  class EAXStructure : public StructuredRegister {
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

  class ECXStructure : public StructuredRegister {
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

  class EDXStructure : public StructuredRegister {
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

  FeatureRegisters() {
    uint32_t ebx = 0;

    __cpuid(0x01, *eax.mutable_raw_value(), ebx, *ecx.mutable_raw_value(),
            *edx.mutable_raw_value());
  }

  EAXStructure eax;
  ECXStructure ecx;
  EDXStructure edx;
};

// Represents the structure of registers when fetching extended features (EAX =
// 7).
struct ExtendedFeatureRegisters {
  class EBXStructure : public StructuredRegister {
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

  class ECXStructure : public StructuredRegister {
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

  ExtendedFeatureRegisters() {
    uint32_t eax = 0;
    uint32_t edx = 0;
    __cpuid(0x07, eax, *ebx.mutable_raw_value(), *ecx.mutable_raw_value(), edx);
  }

  EBXStructure ebx;
  ECXStructure ecx;
};

// Represents the structure of registers when fetching extended features (EAX =
// 80000001H).
struct Extended2FeatureRegisters {
  class ECXStructure : public StructuredRegister {
   public:
    int lahf_sahf() const { return ValueAt<0, 0>(); }
    // 1 - 4 reserved.
    int lzcnt() const { return ValueAt<5, 5>(); }
    // 6 - 7 reserved.
    int prefetchw() const { return ValueAt<8, 8>(); }
    // 9 - 31 reserved.
  };

  class EDXStructure : public StructuredRegister {
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

  Extended2FeatureRegisters() {
    uint32_t eax = 0;
    uint32_t ebx = 0;
    __cpuid(0x80000001, eax, ebx, *ecx.mutable_raw_value(),
            *edx.mutable_raw_value());
  }

  ECXStructure ecx;
  EDXStructure edx;
};

#define PROCESS_FEATURE(name, reg, field)  \
  if (reg.field()) {                       \
    InsertOrDie(&indexed_features, #name); \
  }

HostCpuInfo CreateHostCpuInfo() {
  // Basic checks.
  {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    // Check that we can retrieve extended features.
    __cpuid(0, eax, ebx, ecx, edx);
    CHECK_GT(eax, 0x07);
  }

  const FeatureRegisters features;
  const ExtendedFeatureRegisters ext_features;
  const Extended2FeatureRegisters ext2_features;

  std::unordered_set<string> indexed_features;

  PROCESS_FEATURE(ADX, ext_features.ebx, adx);
  PROCESS_FEATURE(CLFLUSHOPT, ext_features.ebx, clflushopt);
  PROCESS_FEATURE(AES, features.ecx, aes);
  PROCESS_FEATURE(AVX, features.ecx, avx);
  PROCESS_FEATURE(AVX2, ext_features.ebx, avx2);
  PROCESS_FEATURE(BMI1, ext_features.ebx, bmi1);
  PROCESS_FEATURE(BMI2, ext_features.ebx, bmi2);
  PROCESS_FEATURE(PCLMULQDQ, features.ecx, pclmulqdq);
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
  PROCESS_FEATURE(PRFCHW, ext_features.ecx, prefetchwt1);
  PROCESS_FEATURE(RDPID, ext_features.ecx, rdpid);
  PROCESS_FEATURE(RDRAND, features.ecx, rdrand);
  PROCESS_FEATURE(RDSEED, ext_features.ebx, rdseed);
  PROCESS_FEATURE(RTM, ext_features.ebx, rtm);
  PROCESS_FEATURE(SHA, ext_features.ebx, sha);
  PROCESS_FEATURE(SSE, features.edx, sse);
  PROCESS_FEATURE(SSE2, features.edx, sse2);
  PROCESS_FEATURE(SSE3, features.ecx, sse3);
  PROCESS_FEATURE(SSE4_1, features.ecx, sse4_1);
  PROCESS_FEATURE(SSE4_2, features.ecx, sse4_2);
  PROCESS_FEATURE(SSSE3, features.ecx, ssse3);
  PROCESS_FEATURE(XSAVEOPT, features.ecx, xsave);

  // See CPUID doc for the algorithm.
  const int family =
      features.eax.family() != 0x0f
          ? features.eax.family()
          : (features.eax.efamily() << 4) + features.eax.family();
  const int model =
      (features.eax.family() == 0x06 || features.eax.family() == 0x0f)
          ? (features.eax.emodel() << 4) + features.eax.model()
          : features.eax.model();

  return HostCpuInfo(StringPrintf("intel:%02X_%02X", family, model),
                     std::move(indexed_features));
}
#else
// TODO(courbet): Add support for ARM if needed. The above code should work for
// __386__ but I have no way to tell.
#error "cpu architecture not supported"
#endif  // __x86_64__

}  // namespace

const HostCpuInfo& HostCpuInfo::Get() {
  static const auto* const cpu_info = new HostCpuInfo(CreateHostCpuInfo());
  return *cpu_info;
}

HostCpuInfo::HostCpuInfo(const string& id,
                         std::unordered_set<string> indexed_features)
    : cpu_id_(id), indexed_features_(std::move(indexed_features)) {}

bool HostCpuInfo::HasExactFeature(const string& name) const {
  return ContainsKey(indexed_features_, name);
}

// TODO(courbet): Right now strings::Split() does not support a string literal
// delimiter. Switch to it when it does.
template <bool is_or>
bool HostCpuInfo::IsFeatureSet(const string& name, bool* const value) const {
  char kSeparator[5] = " && ";
  if (is_or) {
    kSeparator[1] = kSeparator[2] = '|';
  }
  const char* begin = name.data();
  const char* end = std::strstr(begin, kSeparator);
  if (end == nullptr) {
    return false;  // Not a feature set.
  }

  while (end != nullptr) {
    const bool has_feature = HasExactFeature(string(begin, end));
    if (is_or && has_feature) {
      *value = true;
      return true;
    }
    if (!is_or && !has_feature) {
      *value = false;
      return true;
    }
    begin = end + sizeof(kSeparator) - 1;
    end = std::strstr(begin, kSeparator);
  }
  *value = HasExactFeature(begin);
  return true;
}

bool HostCpuInfo::SupportsFeature(const string& feature_name) const {
  // We don't support parenthesized features combinations for now.
  CHECK(feature_name.find('(') == string::npos);
  CHECK(feature_name.find(')') == string::npos);

  bool value = false;
  if (IsFeatureSet<true>(feature_name, &value) ||
      IsFeatureSet<false>(feature_name, &value)) {
    return value;
  }
  return HasExactFeature(feature_name);
}

string HostCpuInfo::DebugString() const {
  string result = StrCat(cpu_id_, "\nfeatures:");
  for (const auto& feature : indexed_features_) {
    StrAppend(&result, "\n", feature);
  }
  return result;
}
}  // namespace cpu_instructions
