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

#include "exegesis/proto/microarchitecture.pb.h"
#include "exegesis/x86/cpuid.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "strings/string_view_utils.h"

namespace exegesis {
namespace {

using ::exegesis::x86::CpuIdDump;
using ::testing::UnorderedElementsAreArray;
using ::exegesis::util::StatusOr;

void TestFeaturesFromCpuIdDump(
    const std::initializer_list<CpuIdDump::Entry>& dump,
    const std::initializer_list<const char*>& expected_features) {
  const CpuIdDump cpuid_dump(dump);
  ASSERT_TRUE(cpuid_dump.IsValid());
  const StatusOr<CpuInfo> cpu_info_or_status =
      CpuInfo::FromCpuIdDump(cpuid_dump);
  ASSERT_OK(cpu_info_or_status.status());
  const CpuInfo& cpu_info = cpu_info_or_status.ValueOrDie();
  EXPECT_THAT(cpu_info.supported_features(),
              UnorderedElementsAreArray(expected_features));
}

TEST(CpuInfo, FromCpuIdDump_Intel486) {
  TestFeaturesFromCpuIdDump(
      {{{0, 0}, {0x00000001, 0x756e6547, 0x6c65746e, 0x49656e69}},
       {{1, 0}, {0x00000480, 0x00000000, 0x00000000, 0x00000003}}},
      {"FPU"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_PentiumMmx) {
  TestFeaturesFromCpuIdDump(
      {{{0, 0}, {0x00000001, 0x756E6547, 0x6C65746E, 0x49656E69}},
       {{1, 0}, {0x00000543, 0x00000000, 0x00000000, 0x008003BF}}},
      {"FPU", "MMX"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_PentiumIII) {
  TestFeaturesFromCpuIdDump(
      {{{0, 0}, {0x00000003, 0x756E6547, 0x6C65746E, 0x49656E69}},
       {{1, 0}, {0x00000673, 0x00000000, 0x00000000, 0x0387F9FF}},
       {{2, 0}, {0x03020101, 0x00000000, 0x00000000, 0x0C040843}},
       {{3, 0}, {0x00000000, 0x00000000, 0x8EF18AEE, 0x0000D043}}},
      {"FPU", "MMX", "SSE"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_Nehalem) {
  TestFeaturesFromCpuIdDump(
      {{{0x0, 0}, {0x0000000B, 0x756E6547, 0x6C65746E, 0x49656E69}},
       {{0x1, 0}, {0x000106A2, 0x00100800, 0x00BCE3BD, 0xBFEBFBFF}},
       {{0x2, 0}, {0x55035A01, 0x00F0B2E4, 0x00000000, 0x09CA212C}},
       {{0x7, 0}, {0x00000000, 0x00000000, 0x00000000, 0x00000000}},
       {{0x80000001, 0}, {0x00000000, 0x00000000, 0x00000001, 0x28100000}}},
      {"CLFSH", "FPU", "MMX", "SSE", "SSE2", "SSE3", "SSE4_1", "SSE4_2",
       "SSSE3"});
}

TEST(FeatureNamesFromCpuIdTest, FromCpuIdDump_Skylake) {
  TestFeaturesFromCpuIdDump(
      {{{0x0, 0}, {0x00000016, 0x756E6547, 0x6C65746E, 0x49656E69}},
       {{0x1, 0}, {0x000506E3, 0x00100800, 0x7FFAFBBF, 0xBFEBFBFF}},
       {{0x7, 0}, {0x00000000, 0x029C6FBB, 0x00000000, 0x00000000}},
       {{0xD, 0}, {0x0000001F, 0x00000440, 0x00000440, 0x00000000}},
       {{0xD, 1}, {0x0000000F, 0x000003C0, 0x00000100, 0x00000000}},
       {{0xD, 2}, {0x00000100, 0x00000240, 0x00000000, 0x00000000}},
       {{0xD, 3}, {0x00000040, 0x000003C0, 0x00000000, 0x00000000}},
       {{0xD, 4}, {0x00000040, 0x00000400, 0x00000000, 0x00000000}},
       {{0x80000001, 0}, {0x00000000, 0x00000000, 0x00000121, 0x2C100000}}},
      {"ADX",     "AES",   "AVX",     "AVX2",  "BMI1", "BMI2",     "CLFLUSHOPT",
       "CLFSH",   "CLMUL", "FMA",     "F16C",  "FPU",  "FSGSBASE", "HLE",
       "INVPCID", "LZCNT", "MMX",     "MOVBE", "MPX",  "PRFCHW",   "RDRAND",
       "RDSEED",  "RTM",   "SMAP",    "SSE",   "SSE2", "SSE3",     "SSE4_1",
       "SSE4_2",  "SSSE3", "XSAVEOPT"});
}

TEST(CpuInfoTest, Print) {
  const auto cpu_info = CpuInfo::FromHost();
  LOG(INFO) << cpu_info.DebugString();
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
