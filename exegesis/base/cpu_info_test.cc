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

#include "exegesis/base/cpu_info.h"

#include <initializer_list>

#include "exegesis/proto/cpuid.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "exegesis/proto/x86/cpuid.pb.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/x86/cpuid.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "strings/string_view.h"
#include "strings/string_view_utils.h"

namespace exegesis {
namespace {

using ::exegesis::x86::CpuIdDump;
using ::testing::UnorderedElementsAreArray;
using ::exegesis::util::StatusOr;

void TestFeaturesFromCpuIdDump(
    const string& dump_string,
    const std::initializer_list<const char*>& expected_features) {
  const StatusOr<CpuIdDump> dump_or_status = CpuIdDump::FromString(dump_string);
  ASSERT_OK(dump_or_status.status());
  const CpuIdDump& dump = dump_or_status.ValueOrDie();
  ASSERT_TRUE(dump.IsValid());
  const StatusOr<CpuInfo> cpu_info_or_status = CpuInfo::FromCpuIdDump(dump);
  ASSERT_OK(cpu_info_or_status.status());
  const CpuInfo& cpu_info = cpu_info_or_status.ValueOrDie();
  EXPECT_THAT(cpu_info.supported_features(),
              UnorderedElementsAreArray(expected_features));
}

TEST(CpuInfo, FromCpuIdDump_Intel486) {
  TestFeaturesFromCpuIdDump(R"(
      CPUID 00000000: 00000001-756e6547-6c65746e-49656e69
      CPUID 00000001: 00000480-00000000-00000000-00000003)",
                            {"FPU"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_PentiumMmx) {
  TestFeaturesFromCpuIdDump(R"(
      CPUID 00000000: 00000001-756E6547-6C65746E-49656E69
      CPUID 00000001: 00000543-00000000-00000000-008003BF)",
                            {"FPU", "MMX"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_PentiumIII) {
  TestFeaturesFromCpuIdDump(R"(
      CPUID 00000000: 00000003-756E6547-6C65746E-49656E69
      CPUID 00000001: 00000673-00000000-00000000-0387F9FF
      CPUID 00000002: 03020101-00000000-00000000-0C040843
      CPUID 00000003: 00000000-00000000-8EF18AEE-0000D043)",
                            {"FPU", "MMX", "SSE"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_Nehalem) {
  TestFeaturesFromCpuIdDump(R"(
      CPUID 00000000: 0000000B-756E6547-6C65746E-49656E69
      CPUID 00000001: 000106A2-00100800-00BCE3BD-BFEBFBFF
      CPUID 00000002: 55035A01-00F0B2E4-00000000-09CA212C
      CPUID 00000007: 00000000-00000000-00000000-00000000
      CPUID 80000001: 00000000-00000000-00000001-28100000)",
                            {"CLFSH", "FPU", "MMX", "SSE", "SSE2", "SSE3",
                             "SSE4_1", "SSE4_2", "SSSE3"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_Skylake) {
  TestFeaturesFromCpuIdDump(
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
      {"ADX",     "AES",   "AVX",     "AVX2",  "BMI1", "BMI2",     "CLFLUSHOPT",
       "CLFSH",   "CLMUL", "FMA",     "F16C",  "FPU",  "FSGSBASE", "HLE",
       "INVPCID", "LZCNT", "MMX",     "MOVBE", "MPX",  "PRFCHW",   "RDRAND",
       "RDSEED",  "RTM",   "SMAP",    "SSE",   "SSE2", "SSE3",     "SSE4_1",
       "SSE4_2",  "SSSE3", "XSAVEOPT"});
}

TEST(CpuInfoTest, Print) {
  const auto cpu_info = CpuInfo::FromHost();
  EXPECT_TRUE(strings::StartsWith(cpu_info.cpu_id(), "intel:06_"));
}

TEST(CpuInfoTest, SupportsFeature) {
  const CpuInfo cpu_info("doesnotexist", {"ADX", "SSE", "LZCNT"});
  EXPECT_TRUE(cpu_info.SupportsFeature("ADX"));
  EXPECT_TRUE(cpu_info.SupportsFeature("SSE"));
  EXPECT_TRUE(cpu_info.SupportsFeature("LZCNT"));
  EXPECT_FALSE(cpu_info.SupportsFeature("AVX"));

  EXPECT_TRUE(cpu_info.SupportsFeature("ADX || AVX"));

  EXPECT_FALSE(cpu_info.SupportsFeature("ADX && AVX"));
}

}  // namespace
}  // namespace exegesis
