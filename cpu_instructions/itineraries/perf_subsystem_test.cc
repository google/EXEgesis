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

#include "cpu_instructions/itineraries/perf_subsystem.h"

#include <cstdint>

#include "glog/logging.h"
#include "gtest/gtest.h"
#include "strings/str_cat.h"

namespace cpu_instructions {

const uint64_t kIter = 1000;

#define NL "\n\t"

TEST(PerfSubsystemTest, Accumulate) {
  PerfResult r1{{"a", TimingInfo(1, 2, 3)}, {"b", TimingInfo(4, 5, 6)}};
  const string r1_string = PerfResultString(r1);
  EXPECT_EQ("a: 1.50, b: 4.80, ", r1_string);
  PerfResult r2{{"b", TimingInfo(4, 5, 6)}, {"c", TimingInfo(7, 8, 9)}};
  const string r2_string = PerfResultString(r2);
  EXPECT_EQ("b: 4.80, c: 7.88, ", r2_string);
  LOG(INFO) << PerfResultString(r2);
  AccumulateCounters(r1, &r2);
  EXPECT_EQ("a: 1.50, b: 9.60, c: 7.88, ", PerfResultString(r2));
  PerfResult r;
  AccumulateCounters(r, &r1);
  EXPECT_EQ(r1_string, PerfResultString(r1));
  AccumulateCounters(r1, &r);
  EXPECT_EQ(r1_string, PerfResultString(r));
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
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, k = Fib(20););
  EXPECT_EQ(10946, k);
  LOG(INFO) << PerfResultString(result);
}

TEST(PerfSubsystemTest, BasicInlineAsmSyntax) {
  asm volatile(NL "movl %0,%%eax"
               :        /* output" */
               : "I"(3) /*input */
               : "%eax" /* clobbered registers */);
}

TEST(PerfSubsystemTest, CpuId) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "xor %%eax,%%eax"
      NL "cpuid"
      NL ".endr"
      :
      :
      : "%eax", "%ebx", "%ecx", "%edx"));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, Xor) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "xor %%eax,%%eax"
      NL "xor %%ebx,%%ebx"
      NL "xor %%ecx,%%ecx"
      NL "xor %%edx,%%edx"
      NL ".endr"
      :
      :
      : "%eax", "%ebx", "%ecx", "%edx"));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

// CVTPD2PS uses P1 and P5.
// The latency is 4, the reciprocal throughput is 1.
// It is expected that the uop on P1 has a latency of 3, while the one on P5
// (similar to a shuffle) has a latency of 1.
TEST(PerfSubsystemTest, Cvtpd2psLatency) {
  // Latency.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtpd2ps %%xmm0,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL "cvtpd2ps %%xmm1,%%xmm0"  // 3 cycles on port 1, 1 on port 5.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

// Reciprocal throughput = average number of cycles per instruction.
TEST(PerfSubsystemTest, Cvtpd2psReciprocalThroughput) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtpd2ps %%xmm0,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

// CVTDQ2PS

TEST(PerfSubsystemTest, AddXorAdd) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "xor %%r11,%%r11"
      NL "add %%r10,%%r10"
      NL "add %%r10,%%r11"
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

////////////////////////////////////////////

TEST(PerfSubsystemTest, Cvtdq2psShufpd) {
  // Takes 1 cycle on average.
  // t=0: cvtdq2ps is issued on port 1, latency 3.
  // t=0: simultaneously shufpd  on port 5, latency 1.
  //      shufpd clobbers xmm1, cvtdq2ps is aborted.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"

      NL "cvtdq2ps %%xmm0,%%xmm1"           // 3 cycles on port 1.
      NL "shufpd %[shuffle],%%xmm0,%%xmm1"  // 1 cycle on port 5.

      NL ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
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
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"

      NL "cvtpd2ps %%xmm0,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL "shufpd %[shuffle],%%xmm0,%%xmm1"  // 1 cycle on port 5.

      NL ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, Cvtpd2psCvtdq2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"

      NL "cvtpd2ps %%xmm0,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL "cvtdq2ps %%xmm0,%%xmm1"  // 3 cycles on port 1.

      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

// CVTSI2SD uses P1 and P5.
// The latency is 4, the reciprocal throughput is 1.
// It is expected that the uop on P1 has a latency of 3, while the one on P5
// (similar to a shuffle) has a latency of 1.
TEST(PerfSubsystemTest, Cvtsd2siLatency) {
  // Latency.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtsi2sd %%eax,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL "cvtsd2si %%xmm1,%%eax"  // 3 cycles on port 1, 1 on port 5.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

// Reciprocal throughput = average number of cycles per instruction.
TEST(PerfSubsystemTest, Cvtsd2siReciprocalThroughput) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtsi2sd %%eax,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
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
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtsi2sdq %%rax,%%xmm1"  //  1 cycles on port 1, 3 on port 5.
      NL "shufpd %[shuffle],%%xmm0,%%xmm1"  // 1 cycle on port 5.
      NL ".endr"
      :
      : [shuffle] "I"(15)
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, Cvtsi2sdCvtdq2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtsi2sd %%rax,%%xmm1"  // 1 cycles on port 1, 3 on port 5.
      NL "cvtdq2ps %%xmm0,%%xmm1"  // 3 cycles on port 1.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, Cvtdq2psCvtpd2ps) {
  // 2 cycles average.
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "cvtdq2ps %%xmm0,%%xmm1"  // 3 cycles on port 1.
      NL "cvtpd2ps %%xmm0,%%xmm1"  // 3 cycles on port 1, 1 on port 5.
      NL ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, Shufpd) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "shufpd %[shuffle],%%xmm0,%%xmm1"  // 1 cycle on port 5.
      NL "shufpd %[shuffle],%%xmm1,%%xmm0"  // 1 cycle on port 5.
      NL ".endr"
      :
      : [shuffle] "I"(3)));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, MOV64mi32) {
  PerfResult result;
  uint64_t memory;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "movl $123, %[memory]"
      NL ".endr"
      :
      : [memory] "m"(memory)));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, ADDSDrm) {
  PerfResult result;
  double memory;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL ".rept 1000"
      NL "addsd %[memory], %%xmm0"
      NL ".endr"
      :
      : [memory] "m"(memory)
      : "%xmm0"));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

#ifndef MEMORY_SANITIZER
TEST(PerfSubsystemTest, ADDSDrmSize) {
  double memory;
  asm volatile(NL "movsd %%xmm0,%[memory]" : [memory] "=m"(memory) : :);
  EXPECT_NE("", StrCat(memory));
}
#endif

TEST(PerfSubsystemTest, BlockThroughput) {
  // This was extracted from CapProdWithDoubles code.
  PerfResult result;
  uint64_t memory;
  uint64_t address = reinterpret_cast<uint64_t>(&memory);
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL "movq %[address], %%rsi"
      NL ".rept 1000"
      NL "cvtsi2sdq %%rdi, %%xmm2"
      NL "movsd (%%rsi), %%xmm1"
      NL "andpd %%xmm1, %%xmm2"
      NL "movsd (%%rsi),%%xmm0"
      NL "movaps %%xmm2, %%xmm3"
      NL "subsd %%xmm0, %%xmm3"
      NL ".endr"
      :
      : [address] "r"(address)
      : "%rsi", "%xmm0", "%xmm1", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << PerfResultString(result, 1000);
}

TEST(PerfSubsystemTest, LoopDetectorJAE) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL "mov $0xFFFF, %%ecx"
      "\n1:"
      NL "cvtsi2sdq %%rdi, %%xmm2"
      NL "cvtsi2sdq %%rsp, %%xmm3"
      NL "decl %%ecx"
      NL "cmpl $0x1, %%ecx"
      NL "jae 1b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << PerfResultString(result);
}

TEST(PerfSubsystemTest, LoopDetectorJNE) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL "mov $0xFFFF, %%rcx"
      "\n2:"
      NL "cvtsi2sdq %%rdi, %%xmm2"
      NL "cvtsi2sdq %%rsp, %%xmm3"
      NL "dec %%rcx"
      NL "jne 2b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << PerfResultString(result);
}

TEST(PerfSubsystemTest, LoopDetectorJLE) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL "xor %%rcx, %%rcx"
      "\n2:"
      NL "cvtsi2sdq %%rdi, %%xmm2"
      NL "cvtsi2sdq %%rsp, %%xmm3"
      NL "inc %%rcx"
      NL "cmpq $0xFFFF, %%rcx"
      NL "jle 2b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << PerfResultString(result);
}

TEST(PerfSubsystemTest, LoopDetectorJL) {
  PerfResult result;
  // clang-format off
  CPU_INSTRUCTIONS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      NL "xor %%rcx, %%rcx"
      "\n2:"
      NL "cvtsi2sdq %%rdi, %%xmm2"
      NL "cvtsi2sdq %%rsp, %%xmm3"
      NL "inc %%rcx"
      NL "cmpq $0xFFFF, %%rcx"
      NL "jl 2b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << PerfResultString(result);
}

}  // namespace cpu_instructions
