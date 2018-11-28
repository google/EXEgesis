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

#include <initializer_list>
#include <string>
#include <unordered_set>
#include <utility>

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::StatusOr;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::UnorderedElementsAreArray;

constexpr char kDumpString[] = R"(
    This line is a comment. It does not have any effect on the result.

    CPUID 00000000: 00000016-756E6547-6C65746E-49656E69 [GenuineIntel]
    CPUID 00000001: 000906E9-00100800-4FFAEBBF-BFEBFBFF
    CPUID 00000002: 76036301-00F0B5FF-00000000-00C30000
    CPUID 00000003: 00000000-00000000-00000000-00000000
    CPUID 00000004: 1C004121-01C0003F-0000003F-00000000 [SL 00]
    CPUID 00000004: 1C004122-01C0003F-0000003F-00000000 [SL 01]
    CPUID 00000004: 1C004143-00C0003F-000003FF-00000000 [SL 02]
    CPUID 00000004: 1C03C163-02C0003F-00000FFF-00000006 [SL 03]
    CPUID 00000005: 00000040-00000040-00000003-00142120
    CPUID 00000006: 000027F5-00000002-00000001-00000000
    CPUID 00000007: 00000000-02946687-00000000-00000000
    CPUID 00000008: 00000000-00000000-00000000-00000000
    CPUID 00000009: 00000000-00000000-00000000-00000000
    CPUID 0000000A: 07300404-00000000-00000000-00000603
    CPUID 0000000B: 00000001-00000002-00000100-00000000
    CPUID 0000000B: 00000004-00000004-00000201-00000000
    CPUID 0000000C: 00000000-00000000-00000000-00000000
    CPUID 0000000D: 0000001B-00000440-00000440-00000000
    CPUID 0000000D: 0000000F-000002C0-00000100-00000000
    CPUID 0000000E: 00000000-00000000-00000000-00000000
    CPUID 0000000F: 00000000-00000000-00000000-00000000
    CPUID 0000000F: 00000000-00000000-00000000-00000000
    CPUID 00000010: 00000000-00000000-00000000-00000000
    CPUID 00000010: 00000000-00000000-00000000-00000000
    CPUID 00000011: 00000000-00000000-00000000-00000000
    CPUID 00000012: 00000000-00000000-00000000-00000000
    CPUID 00000012: 00000000-00000000-00000000-00000000
    CPUID 00000013: 00000000-00000000-00000000-00000000
    CPUID 00000014: 00000001-0000000F-00000007-00000000
    CPUID 00000014: 02490002-003F3FFF-00000000-00000000
    CPUID 00000015: 00000002-0000012C-00000000-00000000
    CPUID 00000016: 00000E10-00000E10-00000064-00000000
    CPUID 80000000: 80000008-00000000-00000000-00000000
    CPUID 80000001: 00000000-00000000-00000121-2C100000
    CPUID 80000002: 65746E49-2952286C-6E655020-6D756974
    CPUID 80000003: 20295228-20555043-30363447-20402030
    CPUID 80000008: 00003027-00000000-00000000-00000000)";

TEST(MakeCpuIdEntryTest, MakeEntry) {
  const CpuIdEntryProto entry = MakeCpuIdEntry(1, 2, 3, 4, 5, 6);
  EXPECT_EQ(entry.input().leaf(), 1);
  EXPECT_EQ(entry.input().subleaf(), 2);
  EXPECT_EQ(entry.output().eax(), 3);
  EXPECT_EQ(entry.output().ebx(), 4);
  EXPECT_EQ(entry.output().ecx(), 5);
  EXPECT_EQ(entry.output().edx(), 6);
}

#ifdef __x86_64__
TEST(CpuIdDumpTest, FromHost) {
  const CpuIdDump dump = CpuIdDump::FromHost();
  ASSERT_TRUE(dump.IsValid());
  EXPECT_THAT(dump.GetVendorString(), Not(IsEmpty()));
  EXPECT_THAT(dump.GetProcessorBrandString(), Not(IsEmpty()));
  const CpuIdDumpProto& dump_proto = dump.dump_proto();
  ASSERT_TRUE(dump_proto.has_x86_cpuid_dump());
  // Checks that all leafs and subleafs are returned only once in the proto.
  std::set<std::pair<uint32_t, uint32_t>> leafs_and_subleafs;
  for (const auto& entry : dump_proto.x86_cpuid_dump().entries()) {
    const std::pair<uint32_t, uint32_t> leaf_and_subleaf =
        std::make_pair(entry.input().leaf(), entry.input().subleaf());
    EXPECT_TRUE(gtl::InsertIfNotPresent(&leafs_and_subleafs, leaf_and_subleaf));
  }
}
#endif  // __x86_64__

TEST(CpuIdDumpTest, DefaultConstructor) {
  const CpuIdDump dump;
  EXPECT_FALSE(dump.IsValid());
}

TEST(CpuIdDumpTest, VendorAndBrandString) {
  const StatusOr<CpuIdDump> dump_or_status = CpuIdDump::FromString(R"(
      CPUID 00000000: 00000016-756E6547-6C65746E-49656E69
      CPUID 00000001: 000906E9-00100800-7FFAFBBF-BFEBFBFF
      CPUID 00000007: 00000000-029C6FBF-00000000-00000000
      CPUID 0000000D: 0000001F-00000440-00000440-00000000
      CPUID 0000000D: 0000000F-000003C0-00000100-00000000
      CPUID 0000000D: 00000100-00000240-00000000-00000000
      CPUID 0000000D: 00000040-000003C0-00000000-00000000
      CPUID 0000000D: 00000040-00000400-00000000-00000000
      CPUID 80000000: 80000008-00000000-00000000-00000000
      CPUID 80000001: 00000000-00000000-00000121-2C100000
      CPUID 80000002: 65746E49-2952286C-726F4320-4D542865
      CPUID 80000003: 35692029-3036372D-43204B30-40205550
      CPUID 80000004: 382E3320-7A484730-00000000-00000000
      )");
  ASSERT_OK(dump_or_status.status());
  const CpuIdDump& dump = dump_or_status.ValueOrDie();
  ASSERT_TRUE(dump.IsValid());
  EXPECT_EQ(dump.GetVendorString(), "GenuineIntel");
  EXPECT_EQ(dump.GetProcessorBrandString(),
            "Intel(R) Core(TM) i5-7600K CPU @ 3.80GHz");
}

TEST(CpuIdDumpTest, FromProto) {
  const CpuIdDumpProto kDump =
      ParseProtoFromStringOrDie<CpuIdDumpProto>(R"proto(
        x86_cpuid_dump {
          entries {
            input { leaf: 0 }
            output {
              eax: 0x00000001
              ebx: 0x756e6547
              ecx: 0x6c65746e
              edx: 0x49656e69
            }
          }
          entries {
            input { leaf: 1 }
            output {
              eax: 0x00000480
              ebx: 0x00000000
              ecx: 0x00000000
              edx: 0x00000003
            }
          }
        })proto");
  const CpuIdDump dump(kDump);
  EXPECT_THAT(dump.dump_proto(), EqualsProto(kDump));
  ASSERT_TRUE(dump.IsValid());
  EXPECT_EQ(dump.GetVendorString(), "GenuineIntel");
  const CpuIdOutputProto* const entry = dump.GetEntryOrNull(1);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->eax(), 0x00000480);
  EXPECT_EQ(entry->ebx(), 0x00000000);
  EXPECT_EQ(entry->ecx(), 0x00000000);
  EXPECT_EQ(entry->edx(), 0x00000003);
}

TEST(CpuIdDumpTest, FromEmptyString) {
  constexpr char kDump[] = R"()";
  EXPECT_THAT(CpuIdDump::FromString(kDump), StatusIs(INVALID_ARGUMENT));
}

TEST(CpuIdDumpTest, FromString) {
  const StatusOr<CpuIdDump> dump_or_status = CpuIdDump::FromString(kDumpString);
  ASSERT_OK(dump_or_status.status());
  const CpuIdDump& dump = dump_or_status.ValueOrDie();

  EXPECT_EQ(dump.GetVendorString(), "GenuineIntel");

  // Check that sub-leafs are numbered correctly.
  const CpuIdOutputProto* const with_subleaf = dump.GetEntryOrNull(4, 3);
  ASSERT_NE(with_subleaf, nullptr);
  EXPECT_EQ(with_subleaf->eax(), 0x1c03c163);
  EXPECT_EQ(with_subleaf->ebx(), 0x02c0003f);
  EXPECT_EQ(with_subleaf->ecx(), 0x00000fff);
  EXPECT_EQ(with_subleaf->edx(), 0x00000006);
}

void TestToCpuInfo(
    const std::string& dump_string, const std::string& expected_cpu_model,
    const std::initializer_list<const char*>& expected_features) {
  const StatusOr<CpuIdDump> dump_or_status = CpuIdDump::FromString(dump_string);
  ASSERT_OK(dump_or_status.status());
  const CpuIdDump& dump = dump_or_status.ValueOrDie();
  ASSERT_TRUE(dump.IsValid());
  const CpuInfo cpu_info = dump.ToCpuInfo();
  EXPECT_EQ(cpu_info.cpu_model_id(), expected_cpu_model);
  EXPECT_THAT(cpu_info.supported_features(),
              UnorderedElementsAreArray(expected_features));
}

TEST(CpuIdDumpTest, ToCpuInfo_Intel486) {
  TestToCpuInfo(
      R"(
      CPUID 00000000: 00000001-756e6547-6c65746e-49656e69
      CPUID 00000001: 00000480-00000000-00000000-00000003)",
      "intel:04_08", {"FPU"});
}

TEST(CpuIdDumpTest, ToCpuInfo_PentiumMmx) {
  TestToCpuInfo(
      R"(
      CPUID 00000000: 00000001-756E6547-6C65746E-49656E69
      CPUID 00000001: 00000543-00000000-00000000-008003BF)",
      "intel:05_04", {"FPU", "MMX"});
}

TEST(CpuIdDumpTest, ToCpuInfo_PentiumIII) {
  TestToCpuInfo(
      R"(
      CPUID 00000000: 00000003-756E6547-6C65746E-49656E69
      CPUID 00000001: 00000673-00000000-00000000-0387F9FF
      CPUID 00000002: 03020101-00000000-00000000-0C040843
      CPUID 00000003: 00000000-00000000-8EF18AEE-0000D043)",
      "intel:06_07", {"FPU", "MMX", "SSE"});
}

TEST(CpuIdDumpTest, ToCpuInfo_Nehalem) {
  TestToCpuInfo(
      R"(
      CPUID 00000000: 0000000B-756E6547-6C65746E-49656E69
      CPUID 00000001: 000106A2-00100800-00BCE3BD-BFEBFBFF
      CPUID 00000002: 55035A01-00F0B2E4-00000000-09CA212C
      CPUID 00000007: 00000000-00000000-00000000-00000000
      CPUID 80000001: 00000000-00000000-00000001-28100000)",
      "intel:06_1A",
      {"CLFSH", "FPU", "MMX", "SSE", "SSE2", "SSE3", "SSE4_1", "SSE4_2",
       "SSSE3"});
}

TEST(CpuIdDumpTest, ToCpuInfo_Skylake) {
  TestToCpuInfo(
      R"(
      CPUID 00000000: 00000016-756E6547-6C65746E-49656E69
      CPUID 00000001: 000506E3-00100800-7FFAFBBF-BFEBFBFF
      CPUID 00000007: 00000000-029C6FBB-00000000-00000000
      CPUID 0000000D: 0000001F-00000440-00000440-00000000
      CPUID 0000000D: 0000000F-000003C0-00000100-00000000
      CPUID 0000000D: 00000100-00000240-00000000-00000000
      CPUID 0000000D: 00000040-000003C0-00000000-00000000
      CPUID 0000000D: 00000040-00000400-00000000-00000000
      CPUID 80000001: 00000000-00000000-00000121-2C100000)",
      "intel:06_5E",
      {"ADX",        "AES",       "AVX",     "AVX2",   "BMI1",   "BMI2",
       "CLFLUSHOPT", "CLFSH",     "CLMUL",   "FMA",    "F16C",   "FPU",
       "FSGSBASE",   "HLE",       "INVPCID", "LZCNT",  "MMX",    "MOVBE",
       "MPX",        "PREFETCHW", "RDRAND",  "RDSEED", "RTM",    "SMAP",
       "SSE",        "SSE2",      "SSE3",    "SSE4_1", "SSE4_2", "SSSE3",
       "XSAVEOPT"});
}

TEST(CpuIdDumpTest, ToString) {
  constexpr char kExpectedDumpString[] =
      "CPUID 00000000: 00000016-756E6547-6C65746E-49656E69\n"
      "CPUID 00000001: 000906E9-00100800-4FFAEBBF-BFEBFBFF\n"
      "CPUID 00000002: 76036301-00F0B5FF-00000000-00C30000\n"
      "CPUID 00000003: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000004: 1C004121-01C0003F-0000003F-00000000\n"
      "CPUID 00000004: 1C004122-01C0003F-0000003F-00000000\n"
      "CPUID 00000004: 1C004143-00C0003F-000003FF-00000000\n"
      "CPUID 00000004: 1C03C163-02C0003F-00000FFF-00000006\n"
      "CPUID 00000005: 00000040-00000040-00000003-00142120\n"
      "CPUID 00000006: 000027F5-00000002-00000001-00000000\n"
      "CPUID 00000007: 00000000-02946687-00000000-00000000\n"
      "CPUID 00000008: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000009: 00000000-00000000-00000000-00000000\n"
      "CPUID 0000000A: 07300404-00000000-00000000-00000603\n"
      "CPUID 0000000B: 00000001-00000002-00000100-00000000\n"
      "CPUID 0000000B: 00000004-00000004-00000201-00000000\n"
      "CPUID 0000000C: 00000000-00000000-00000000-00000000\n"
      "CPUID 0000000D: 0000001B-00000440-00000440-00000000\n"
      "CPUID 0000000D: 0000000F-000002C0-00000100-00000000\n"
      "CPUID 0000000E: 00000000-00000000-00000000-00000000\n"
      "CPUID 0000000F: 00000000-00000000-00000000-00000000\n"
      "CPUID 0000000F: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000010: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000010: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000011: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000012: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000012: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000013: 00000000-00000000-00000000-00000000\n"
      "CPUID 00000014: 00000001-0000000F-00000007-00000000\n"
      "CPUID 00000014: 02490002-003F3FFF-00000000-00000000\n"
      "CPUID 00000015: 00000002-0000012C-00000000-00000000\n"
      "CPUID 00000016: 00000E10-00000E10-00000064-00000000\n"
      "CPUID 80000000: 80000008-00000000-00000000-00000000\n"
      "CPUID 80000001: 00000000-00000000-00000121-2C100000\n"
      "CPUID 80000002: 65746E49-2952286C-6E655020-6D756974\n"
      "CPUID 80000003: 20295228-20555043-30363447-20402030\n"
      "CPUID 80000008: 00003027-00000000-00000000-00000000";
  const StatusOr<CpuIdDump> dump_or_status = CpuIdDump::FromString(kDumpString);
  ASSERT_OK(dump_or_status.status());
  const CpuIdDump& dump = dump_or_status.ValueOrDie();
  ASSERT_TRUE(dump.IsValid());
  EXPECT_EQ(dump.ToString(), kExpectedDumpString);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
