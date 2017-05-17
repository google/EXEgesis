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

#include "exegesis/proto/microarchitecture.pb.h"
#include "glog/logging.h"
#include "gtest/gtest.h"
#include "strings/string_view_utils.h"

namespace cpu_instructions {
namespace {

TEST(HostCpuInfoTest, Print) {
  const auto cpu_info = HostCpuInfo::Get();
  LOG(INFO) << cpu_info.DebugString();
  EXPECT_TRUE(strings::StartsWith(cpu_info.cpu_id(), "intel:06_"));
}

TEST(HostCpuInfoTest, SupportsFeature) {
  const HostCpuInfo cpu_info("doesnotexist", {"ADX", "SSE", "LZCNT"});
  EXPECT_TRUE(cpu_info.SupportsFeature("ADX"));
  EXPECT_TRUE(cpu_info.SupportsFeature("SSE"));
  EXPECT_TRUE(cpu_info.SupportsFeature("LZCNT"));
  EXPECT_FALSE(cpu_info.SupportsFeature("AVX"));

  EXPECT_TRUE(cpu_info.SupportsFeature("ADX || AVX"));

  EXPECT_FALSE(cpu_info.SupportsFeature("ADX && AVX"));
}
}  // namespace
}  // namespace cpu_instructions
