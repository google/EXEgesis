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

#include "exegesis/base/cpu_info.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::testing::UnorderedElementsAreArray;

TEST(CpuInfoFromCpuIdDumpTest, X86) {
  const CpuIdDumpProto kDump =
      ParseProtoFromStringOrDie<CpuIdDumpProto>(R"proto(
        x86_cpuid_dump {
          entries {
            input { leaf: 0x00000000 }
            output {
              eax: 0x00000016
              ebx: 0x756E6547
              ecx: 0x6C65746E
              edx: 0x49656E69
            }
          }
          entries {
            input { leaf: 0x00000001 }
            output {
              eax: 0x000506E3
              ebx: 0x00100800
              ecx: 0x7FFAFBBF
              edx: 0xBFEBFBFF
            }
          }
          entries {
            input { leaf: 0x00000007 }
            output {
              eax: 0x00000000
              ebx: 0x029C6FBB
              ecx: 0x00000000
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x0000000D }
            output {
              eax: 0x0000001F
              ebx: 0x00000440
              ecx: 0x00000440
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x0000000D subleaf: 1 }
            output {
              eax: 0x0000000F
              ebx: 0x000003C0
              ecx: 0x00000100
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x0000000D subleaf: 2 }
            output {
              eax: 0x00000100
              ebx: 0x00000240
              ecx: 0x00000000
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x0000000D subleaf: 3 }
            output {
              eax: 0x00000040
              ebx: 0x000003C0
              ecx: 0x00000000
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x0000000D subleaf: 4 }
            output {
              eax: 0x00000040
              ebx: 0x00000400
              ecx: 0x00000000
              edx: 0x00000000
            }
          }
          entries {
            input { leaf: 0x80000001 }
            output {
              eax: 0x00000000
              ebx: 0x00000000
              ecx: 0x00000121
              edx: 0x2C100000
            }
          }
        })proto");
  constexpr char kExpectedCpuId[] = "intel:06_5E";
  constexpr const char* const kExpectedFeatures[] = {
      "ADX",     "AES",   "AVX",     "AVX2",  "BMI1", "BMI2",      "CLFLUSHOPT",
      "CLFSH",   "CLMUL", "FMA",     "F16C",  "FPU",  "FSGSBASE",  "HLE",
      "INVPCID", "LZCNT", "MMX",     "MOVBE", "MPX",  "PREFETCHW", "RDRAND",
      "RDSEED",  "RTM",   "SMAP",    "SSE",   "SSE2", "SSE3",      "SSE4_1",
      "SSE4_2",  "SSSE3", "XSAVEOPT"};
  const CpuInfo cpu_info = CpuInfoFromCpuIdDump(kDump);
  EXPECT_EQ(cpu_info.cpu_model_id(), kExpectedCpuId);
  EXPECT_THAT(cpu_info.supported_features(),
              UnorderedElementsAreArray(kExpectedFeatures));
}

}  // namespace
}  // namespace exegesis
