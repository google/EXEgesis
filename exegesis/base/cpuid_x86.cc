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

#ifdef __x86_64__
#include <cpuid.h>
#endif  //  __x86_64__

#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "exegesis/proto/x86/cpuid.pb.h"
#include "exegesis/util/structured_register.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {

// The CPUID code is based on the information provided in:
// * Intel 64 and IA-32 Architectures Software Developer's Manual (March 2017),
//   combined volumes 1-4, and
// * AMD64 Architecture Programmer's Manual, Volume 3 (March 2017, rev. 3.23).
//
// As of 2017-06-09, the information returned by CPUID is - for the purposes of
// this code - almost equivalent on both platforms. The following differences
// are relevant to this library:
// * ExtendedFeatureRegisters.ecx.prefetchw1() is marked as reserved on AMD.
// * The bits used to indicate AVX512 support are marked as reserved on AMD.

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
    // 12 is reserved ond AMD CPUs.
    int intel_only_type() const { return ValueAt<13, 12>(); }
    // 15 - 14 reserved.
    int emodel() const { return ValueAt<19, 16>(); }
    int efamily() const { return ValueAt<27, 20>(); }
    // 31 - 28 reserved.
  };

  class ECXStructure : public StructuredRegister<uint32_t> {
   public:
    int sse3() const { return ValueAt<0, 0>(); }
    int pclmulqdq() const { return ValueAt<1, 1>(); }
    // 2 is reserved on AMD CPUs.
    int intel_only_dtes64() const { return ValueAt<2, 2>(); }
    int monitor() const { return ValueAt<3, 3>(); }
    // 4 - 8 are reserved on AMD CPUs.
    int intel_only_dscpl() const { return ValueAt<4, 4>(); }
    int intel_only_vmx() const { return ValueAt<5, 5>(); }
    int intel_only_smx() const { return ValueAt<6, 6>(); }
    int intel_only_est() const { return ValueAt<7, 7>(); }
    int intel_only_tm2() const { return ValueAt<8, 8>(); }
    int ssse3() const { return ValueAt<9, 9>(); }
    // 10 - 11 are reserved on AMD CPUs.
    int intel_only_cntxid() const { return ValueAt<10, 10>(); }
    int intel_only_sdbg() const { return ValueAt<11, 11>(); }
    int fma() const { return ValueAt<12, 12>(); }
    int cx16() const { return ValueAt<13, 13>(); }
    // 14 - 18 are reserved on AMD CPUs.
    int intel_only_xtpr() const { return ValueAt<14, 14>(); }
    int intel_only_pdcm() const { return ValueAt<15, 15>(); }
    // 16 reserved.
    int intel_only_pcid() const { return ValueAt<17, 17>(); }
    int intel_only_dca() const { return ValueAt<18, 18>(); }
    int sse4_1() const { return ValueAt<19, 19>(); }
    int sse4_2() const { return ValueAt<20, 20>(); }
    // 21 is reserved on AMD CPUs.
    int intel_only_x2apic() const { return ValueAt<21, 21>(); }
    int movbe() const { return ValueAt<22, 22>(); }
    int popcnt() const { return ValueAt<23, 23>(); }
    // 24 is reserved on AMD CPUs.
    int intel_only_tscdadline() const { return ValueAt<24, 24>(); }
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
    // 18 is reserved on AMD CPUs.
    int intel_only_psn() const { return ValueAt<18, 18>(); }
    int clfsh() const { return ValueAt<19, 19>(); }
    // 20 reserved.
    // 20 - 22 is reserved on AMD CPUs.
    int intel_only_ds() const { return ValueAt<21, 21>(); }
    int intel_only_acpi() const { return ValueAt<22, 22>(); }
    int mmx() const { return ValueAt<23, 23>(); }
    int fxsr() const { return ValueAt<24, 24>(); }
    int sse() const { return ValueAt<25, 25>(); }
    int sse2() const { return ValueAt<26, 26>(); }
    // 27 is reserved on AMD CPUs.
    int intel_only_ss() const { return ValueAt<27, 27>(); }
    int htt() const { return ValueAt<28, 28>(); }
    // 29 - 31 is reserved on AMD CPUs.
    int intel_only_tm() const { return ValueAt<29, 29>(); }
    int intel_only_ia64() const { return ValueAt<30, 30>(); }
    int intel_only_pbe() const { return ValueAt<31, 31>(); }
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
    // 1 - 2 is reserved on AMD CPUs.
    int intel_only_ia32tscadjust() const { return ValueAt<1, 1>(); }
    int intel_only_sgx() const { return ValueAt<2, 2>(); }
    int bmi1() const { return ValueAt<3, 3>(); }
    // 4 is reserved on AMD CPUs.
    int intel_only_hle() const { return ValueAt<4, 4>(); }
    int avx2() const { return ValueAt<5, 5>(); }
    // 6 is reserved.
    int smep() const { return ValueAt<7, 7>(); }
    int bmi2() const { return ValueAt<8, 8>(); }
    // 9 - 17 is reserved on AMD CPUs.
    int intel_only_erms() const { return ValueAt<9, 9>(); }
    int intel_only_invpcid() const { return ValueAt<10, 10>(); }
    int intel_only_rtm() const { return ValueAt<11, 11>(); }
    int intel_only_pqm() const { return ValueAt<12, 12>(); }
    int intel_only_fpucsdsdeprecated() const { return ValueAt<13, 13>(); }
    int intel_only_mpx() const { return ValueAt<14, 14>(); }
    int intel_only_pqe() const { return ValueAt<15, 15>(); }
    int intel_only_avx512f() const { return ValueAt<16, 16>(); }
    int intel_only_avx512dq() const { return ValueAt<17, 17>(); }
    int rdseed() const { return ValueAt<18, 18>(); }
    int adx() const { return ValueAt<19, 19>(); }
    int smap() const { return ValueAt<20, 20>(); }
    // 21 - 22 is reserved on AMD CPUs.
    int intel_only_avx512ifma() const { return ValueAt<21, 21>(); }
    int intel_only_pcommit() const { return ValueAt<22, 22>(); }
    int clflushopt() const { return ValueAt<23, 23>(); }
    // 24 - 28 is reserved on AMD CPUs.
    int intel_only_clwb() const { return ValueAt<24, 24>(); }
    int intel_only_intelproctrace() const { return ValueAt<25, 25>(); }
    int intel_only_avx512pf() const { return ValueAt<26, 26>(); }
    int intel_only_avx512er() const { return ValueAt<27, 27>(); }
    int intel_only_avx512cd() const { return ValueAt<28, 28>(); }
    int sha() const { return ValueAt<29, 29>(); }
    // 30 - 31 is reserved on AMD CPUs.
    int intel_only_avx512bw() const { return ValueAt<30, 30>(); }
    int intel_only_avx512vl() const { return ValueAt<31, 31>(); }
  };

  class ECXStructure : public StructuredRegister<uint32_t> {
   public:
    // 0 - 31 are reserved on AMD CPUs.
    int intel_only_prefetchwt1() const { return ValueAt<0, 0>(); }
    // 1 reserved.
    int intel_only_umip() const { return ValueAt<2, 2>(); }
    int intel_only_pku() const { return ValueAt<3, 3>(); }
    int intel_only_ospke() const { return ValueAt<4, 4>(); }
    // 5 - 21 reserved.
    int intel_only_rdpid() const { return ValueAt<22, 22>(); }
    // 23 - 29 reserved.
    int intel_only_sgx_lc() const { return ValueAt<30, 30>(); }
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
    // 1 - 31 are reserved on AMD CPUs.
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

constexpr int kMaxSubleafStoredInEax = -1;

// Returns the maximal index of a subleaf of the given leaf. For most leafs,
// there is only one subleaf (the main subleaf); for others, the maximal index
// is either stored in the subleaf 0 of that leaf, or the maximal index is
// obtained from the documentation.
// NOTE(ondrasej): This function handles only the cases that are relevant to
// feature detection. There are other leafs that have more than one subleaf that
// are not considered by this function.
int GetMaxSubleaf(int leaf) {
  switch (leaf) {
    case 7:
      return kMaxSubleafStoredInEax;
    case 0x0D:
      return 1;
    default:
      return 0;
  }
}

// Adds leaf from a leaf range starting with 'seed' to 'dump_proto'. Assumes
// that 'seed' is the first leaf of the range, and that the index of the
// last leaf of the range is returned in
// GetHostCpuIdDumpEntry(seed, 0).output().eax().
void AddHostCpuIdEntriesFromSeed(uint32_t seed, X86CpuIdDumpProto* dump_proto) {
  CHECK(dump_proto != nullptr);
  const CpuIdEntryProto seed_leaf = GetHostCpuIdDumpEntry(seed, 0);
  *dump_proto->add_entries() = seed_leaf;
  const uint32_t num_leafs = seed_leaf.output().eax();
  for (uint32_t leaf = seed + 1; leaf <= num_leafs; ++leaf) {
    *dump_proto->add_entries() = GetHostCpuIdDumpEntry(leaf, 0);
    int max_subleaf = GetMaxSubleaf(leaf);
    if (max_subleaf == kMaxSubleafStoredInEax) {
      max_subleaf = dump_proto->entries().rbegin()->output().eax();
    }
    for (int subleaf = 1; subleaf <= max_subleaf; ++subleaf) {
      *dump_proto->add_entries() = GetHostCpuIdDumpEntry(leaf, subleaf);
    }
  }
}

}  // namespace

constexpr char CpuIdDump::kVendorStringAMD[];
constexpr char CpuIdDump::kVendorStringIntel[];

StatusOr<CpuIdDump> CpuIdDump::FromString(const std::string& source) {
  static const LazyRE2 regex = {
      " *CPUID +([0-9a-fA-F]+): +"
      "([0-9a-fA-F]+)-([0-9a-fA-F]+)-([0-9a-fA-F]+)-([0-9a-fA-F]+)(?: .*)?"};
  CpuIdDump dump;
  X86CpuIdDumpProto* const dump_proto =
      dump.dump_proto_.mutable_x86_cpuid_dump();
  const std::vector<std::string> lines = absl::StrSplit(source, '\n');
  for (const std::string& line : lines) {
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
#ifdef __x86_64__
  X86CpuIdDumpProto* const dump_proto =
      dump.dump_proto_.mutable_x86_cpuid_dump();
  AddHostCpuIdEntriesFromSeed(0, dump_proto);
  AddHostCpuIdEntriesFromSeed(0x80000000, dump_proto);
#endif  // __x86_64__
  return dump;
}

std::string CpuIdDump::GetProcessorBrandString() const {
  constexpr uint32_t kStartLeaf = 0x80000002;
  constexpr uint32_t kEndLeaf = 0x80000004;
  constexpr int kBytesPerLeaf = 16;
  // There are 16 bytes of data per leaf (four 32-bit registers), and the leaf
  // range is inclusive on both ends. We reserve space for one additional char
  // at the end of the buffer for a null character that turns the buffer into
  // a valid null-terminated C string if it wasn't already.
  constexpr int kBufferSize = kBytesPerLeaf * (kEndLeaf - kStartLeaf + 1) + 1;
  char buffer[kBufferSize];
  uint32_t* const data = reinterpret_cast<uint32_t*>(buffer);
  int entry = 0;
  for (uint32_t leaf = kStartLeaf; leaf <= kEndLeaf; ++leaf) {
    const CpuIdOutputProto* const leaf_data = GetEntryOrNull(leaf, 0);
    if (leaf_data == nullptr) return std::string();
    data[entry] = leaf_data->eax();
    data[entry + 1] = leaf_data->ebx();
    data[entry + 2] = leaf_data->ecx();
    data[entry + 3] = leaf_data->edx();
    entry += 4;
    CHECK_LT(entry * 4, kBufferSize);
  }
  // Depending on the vendor and model, the data in buffer may or may not be
  // padded with zeros at the end. By adding a sentinel right after the 48 bytes
  // obtained from CPUID, we ensure that the data in buffer is always null
  // terminated, and can be safely used as a C string.
  buffer[kBufferSize - 1] = 0;
  return buffer;
}

std::string CpuIdDump::GetVendorString() const {
  const CpuIdOutputProto* const root = GetEntryOrNull(0, 0);
  CHECK(root) << "Invalid CPUID dump!";
  char buffer[12];
  uint32_t* data = reinterpret_cast<uint32_t*>(buffer);
  data[0] = root->ebx();
  data[1] = root->edx();
  data[2] = root->ecx();
  return std::string(buffer, 12);
}

#define PROCESS_FEATURE(name, reg, field)       \
  if (reg.field()) {                            \
    gtl::InsertOrDie(&indexed_features, #name); \
  }
#define PROCESS_FEATURE_IF(condition, name, reg, field) \
  if ((condition) && reg.field()) {                     \
    gtl::InsertOrDie(&indexed_features, #name);         \
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

  const std::string vendor = GetVendorString();
  const bool is_intel = vendor == kVendorStringIntel;
  const bool is_amd = vendor == kVendorStringAMD;

  absl::flat_hash_set<std::string> indexed_features;

  if (is_intel) {
    PROCESS_FEATURE(3DNOW, ext_features.ecx, intel_only_prefetchwt1);
  } else if (is_amd) {
    indexed_features.insert("3DNOW");
  }
  PROCESS_FEATURE(ADX, ext_features.ebx, adx);
  PROCESS_FEATURE(CLFLUSHOPT, ext_features.ebx, clflushopt);
  PROCESS_FEATURE(AES, features.ecx, aes);
  PROCESS_FEATURE(AVX, features.ecx, avx);
  PROCESS_FEATURE(AVX2, ext_features.ebx, avx2);
  PROCESS_FEATURE_IF(is_intel, AVX512BW, ext_features.ebx, intel_only_avx512bw);
  PROCESS_FEATURE_IF(is_intel, AVX512CD, ext_features.ebx, intel_only_avx512cd);
  PROCESS_FEATURE_IF(is_intel, AVX512DQ, ext_features.ebx, intel_only_avx512dq);
  PROCESS_FEATURE_IF(is_intel, AVX512ER, ext_features.ebx, intel_only_avx512er);
  PROCESS_FEATURE_IF(is_intel, AVX512F, ext_features.ebx, intel_only_avx512f);
  PROCESS_FEATURE_IF(is_intel, AVX512PF, ext_features.ebx, intel_only_avx512pf);
  PROCESS_FEATURE_IF(is_intel, AVX512VL, ext_features.ebx, intel_only_avx512vl);
  PROCESS_FEATURE(BMI1, ext_features.ebx, bmi1);
  PROCESS_FEATURE(BMI2, ext_features.ebx, bmi2);
  PROCESS_FEATURE(CLMUL, features.ecx, pclmulqdq);
  PROCESS_FEATURE(F16C, features.ecx, f16c);
  PROCESS_FEATURE(FMA, features.ecx, fma);
  PROCESS_FEATURE(FPU, features.edx, fpu);
  PROCESS_FEATURE(CLFSH, features.edx, clfsh);
  PROCESS_FEATURE(FSGSBASE, ext_features.ebx, fsgsbase);
  PROCESS_FEATURE_IF(is_intel, HLE, ext_features.ebx, intel_only_hle);
  PROCESS_FEATURE_IF(is_intel, INVPCID, ext_features.ebx, intel_only_invpcid);
  PROCESS_FEATURE(LZCNT, ext2_features.ecx, lzcnt);
  PROCESS_FEATURE(MMX, features.edx, mmx);
  PROCESS_FEATURE(MOVBE, features.ecx, movbe);
  PROCESS_FEATURE_IF(is_intel, MPX, ext_features.ebx, intel_only_mpx);
  PROCESS_FEATURE_IF(is_intel, OSPKE, ext_features.ecx, intel_only_ospke);
  PROCESS_FEATURE(PREFETCHW, ext2_features.ecx, prefetchw);
  PROCESS_FEATURE_IF(is_intel, RDPID, ext_features.ecx, intel_only_rdpid);
  PROCESS_FEATURE(RDRAND, features.ecx, rdrand);
  PROCESS_FEATURE(RDSEED, ext_features.ebx, rdseed);
  PROCESS_FEATURE_IF(is_intel, RTM, ext_features.ebx, intel_only_rtm);
  PROCESS_FEATURE(SHA, ext_features.ebx, sha);
  PROCESS_FEATURE(SMAP, ext_features.ebx, smap);
  PROCESS_FEATURE(SSE, features.edx, sse);
  PROCESS_FEATURE(SSE2, features.edx, sse2);
  PROCESS_FEATURE(SSE3, features.ecx, sse3);
  PROCESS_FEATURE(SSE4_1, features.ecx, sse4_1);
  PROCESS_FEATURE(SSE4_2, features.ecx, sse4_2);
  PROCESS_FEATURE(SSSE3, features.ecx, ssse3);
  PROCESS_FEATURE(XSAVEOPT, ext_state.eax, xsaveopt);

  // If there is any AVX-512 feature, also add a meta-feature AVX512.
  constexpr char kAvx512[] = "AVX512";
  for (const std::string& feature_name : indexed_features) {
    if (absl::StartsWith(feature_name, kAvx512)) {
      indexed_features.insert(kAvx512);
      break;
    }
  }

  // See CPUID doc for the algorithm.
  const int family =
      features.eax.family() != 0x0f
          ? features.eax.family()
          : (features.eax.efamily() << 4) + features.eax.family();
  const int model =
      (features.eax.family() == 0x06 || features.eax.family() == 0x0f)
          ? (features.eax.emodel() << 4) + features.eax.model()
          : features.eax.model();

  CpuInfoProto proto;
  proto.set_model_id(absl::StrFormat("intel:%02X_%02X", family, model));
  for (const auto& feature_name : indexed_features) {
    proto.add_feature_names(feature_name);
  }
  return CpuInfo(proto);
}

#undef PROCESS_FEATURE
#undef PROCESS_FEATURE_IF

std::string CpuIdDump::ToString() const {
  std::string buffer;
  for (const CpuIdEntryProto& entry : dump_proto_.x86_cpuid_dump().entries()) {
    if (!buffer.empty()) buffer += "\n";
    absl::StrAppendFormat(&buffer, "CPUID %08X: %08X-%08X-%08X-%08X",
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
