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

#include "exegesis/itineraries/perf_subsystem.h"

#include <cstdint>

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "strings/str_cat.h"

namespace exegesis {

const uint64_t kIter = 1000;

TEST(PerfSubsystemTest, Accumulate) {
  PerfResult r1({{"a", TimingInfo(1, 2, 3)}, {"b", TimingInfo(4, 5, 6)}});
  const string r1_string = r1.ToString();
  EXPECT_EQ("a: 1.50, b: 4.80, (num_times: 1)", r1_string);
  PerfResult r2({{"b", TimingInfo(4, 5, 6)}, {"c", TimingInfo(7, 8, 9)}});
  const string r2_string = r2.ToString();
  EXPECT_EQ("b: 4.80, c: 7.88, (num_times: 1)", r2_string);
  LOG(INFO) << r2_string;
  r2.Accumulate(r1);
  EXPECT_EQ("a: 1.50, b: 9.60, c: 7.88, (num_times: 1)", r2.ToString());
  PerfResult r;
  r1.Accumulate(r);
  EXPECT_EQ(r1_string, r1.ToString());
  r.Accumulate(r1);
  EXPECT_EQ(r1_string, r.ToString());
}

namespace {
int Fib(int n) {
  if (n < 2) return 1;
  return Fib(n - 1) + Fib(n - 2);
}
}  // namespace

TEST(PerfSubsystemTest, Collect) {
  int k;
  PerfResult result;
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, k = Fib(20););
  EXPECT_EQ(10946, k);
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, BasicInlineAsmSyntax) {
  asm volatile("movl %0,%%eax"
               :        /* output" */
               : "I"(3) /*input */
               : "%eax" /* clobbered registers */);
}

TEST(PerfSubsystemTest, CpuId) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          xor %%eax,%%eax
          cpuid
      .endr)"
      :
      :
      : "%eax", "%ebx", "%ecx", "%edx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Xor) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          xor %%eax,%%eax
          xor %%ebx,%%ebx
          xor %%ecx,%%ecx
          xor %%edx,%%edx
      .endr)"
      :
      :
      : "%eax", "%ebx", "%ecx", "%edx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// CVTPD2PS uses P1 and P5.
// The latency is 4, the reciprocal throughput is 1.
// It is expected that the uop on P1 has a latency of 3, while the one on P5
// (similar to a shuffle) has a latency of 1.
TEST(PerfSubsystemTest, Cvtpd2psLatency) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
          cvtpd2ps %%xmm1,%%xmm0;  # 3 cycles on port 1, 1 on port 5.
       .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// Reciprocal throughput = average number of cycles per instruction.
TEST(PerfSubsystemTest, Cvtpd2psReciprocalThroughput) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          .rept 1000
          cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// CVTDQ2PS

TEST(PerfSubsystemTest, AddXorAdd) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          xor %%r11,%%r11
          add %%r10,%%r10
          add %%r10,%%r11
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

////////////////////////////////////////////

TEST(PerfSubsystemTest, Cvtdq2psShufpd) {
  // Takes 1 cycle on average.
  // t=0: cvtdq2ps is issued on port 1, latency 3.
  // t=0: simultaneously shufpd  on port 5, latency 1.
  //      shufpd clobbers xmm1, cvtdq2ps is aborted.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtdq2ps %%xmm0,%%xmm1;           # 3 cycles on port 1.
          shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.
      .endr)"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtpd2psShufpd) {
  // Takes 2 cycles on average.
  // t=0: cvtpd2ps first issues uop on port 5, latency 1.
  // t=1: cvtpd2ps first issues uop on port 1, latency 3.
  // t=1: simultaneously shufpd issues uop on port 5, latency 1.
  //      shufpd clobbers xmm1, cvtpd2ps is aborted.
  // Total: 2 cycles.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtpd2ps %%xmm0,%%xmm1;          # 3 cycles on port 1, 1 on port 5.
         shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.
      .endr)"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtpd2psCvtdq2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
          cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// CVTSI2SD uses P1 and P5.
// The latency is 4, the reciprocal throughput is 1.
// It is expected that the uop on P1 has a latency of 3, while the one on P5
// (similar to a shuffle) has a latency of 1.
TEST(PerfSubsystemTest, Cvtsd2siLatency) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtsi2sd %%eax,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
          cvtsd2si %%xmm1,%%eax;  # 3 cycles on port 1, 1 on port 5.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// Reciprocal throughput = average number of cycles per instruction.
TEST(PerfSubsystemTest, Cvtsd2siReciprocalThroughput) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtsi2sd %%eax,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsi2sdShufpd) {
  // Takes 4 cycles on average.
  // t=0: cvtsi2sdq first issues uop on port 5, latency 3.
  // t=3: cvtsi2sdq first issues uop on port 1, latency 1.
  // t=3: simultaneously shufpd issues uop on port 5, latency 1.
  //      shufpd clobbers xmm1, cvtsi2sdq is aborted.
  // Total: 2 cycles.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtsi2sdq %%rax,%%xmm1;           # 1 cycle on port 1, 3 on port 5.
          shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.
      .endr)"
      :
      : [shuffle] "I"(15)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsi2sdCvtdq2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtsi2sd %%rax,%%xmm1 ;  # 1 cycle on port 1, 3 on port 5.
          cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtdq2psCvtpd2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.
          cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.
      .endr)"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Shufpd) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.
          shufpd %[shuffle],%%xmm1,%%xmm0;  # 1 cycle on port 5.
      .endr)"
      :
      : [shuffle] "I"(3)));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, MOV64mi32) {
  PerfResult result;
  uint64_t memory;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          movl $123, %[memory]
      .endr)"
      :
      : [memory] "m"(memory)));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, ADDSDrm) {
  PerfResult result;
  double memory;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
      .rept 1000
          addsd %[memory], %%xmm0
      .endr)"
      :
      : [memory] "m"(memory)
      : "%xmm0"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

#ifndef MEMORY_SANITIZER
TEST(PerfSubsystemTest, ADDSDrmSize) {
  double memory;
  asm volatile("movsd %%xmm0,%[memory]" : [memory] "=m"(memory) : :);
  EXPECT_NE("", StrCat(memory));
}
#endif

TEST(PerfSubsystemTest, BlockThroughput) {
  // This was extracted from CapProdWithDoubles code.
  PerfResult result;
  uint64_t memory;
  uint64_t address = reinterpret_cast<uint64_t>(&memory);
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          movq %[address], %%rsi
      .rept 1000
          cvtsi2sdq %%rdi, %%xmm2
          movsd (%%rsi), %%xmm1
          andpd %%xmm1, %%xmm2
          movsd (%%rsi),%%xmm0
          movaps %%xmm2, %%xmm3
          subsd %%xmm0, %%xmm3
      .endr)"
      :
      : [address] "r"(address)
      : "%rsi", "%xmm0", "%xmm1", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJAE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          mov $0xFFFF, %%ecx
      1:
          cvtsi2sdq %%rdi, %%xmm2
          cvtsi2sdq %%rsp, %%xmm3
          decl %%ecx
          cmpl $0x1, %%ecx
          jae 1b)"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJNE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          mov $0xFFFF, %%rcx
      2:
          cvtsi2sdq %%rdi, %%xmm2
          cvtsi2sdq %%rsp, %%xmm3
          dec %%rcx
          jne 2b)"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJLE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          xor %%rcx, %%rcx
      2:
          cvtsi2sdq %%rdi, %%xmm2
          cvtsi2sdq %%rsp, %%xmm3
          inc %%rcx
          cmpq $0xFFFF, %%rcx
          jle 2b)"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJL) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(R"(
          xor %%rcx, %%rcx
      2:
          cvtsi2sdq %%rdi, %%xmm2
          cvtsi2sdq %%rsp, %%xmm3
          inc %%rcx
          cmpq $0xFFFF, %%rcx
          jl 2b)"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

}  // namespace exegesis
