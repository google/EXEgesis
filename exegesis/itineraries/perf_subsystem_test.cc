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

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

namespace exegesis {

const uint64_t kIter = 1000;

TEST(PerfSubsystemTest, Accumulate) {
  PerfResult r1({{"a", TimingInfo(1, 2, 3)}, {"b", TimingInfo(4, 5, 6)}});
  const std::string r1_string = r1.ToString();
  EXPECT_EQ("a: 1.50, b: 4.80, (num_times: 1)", r1_string);
  PerfResult r2({{"b", TimingInfo(4, 5, 6)}, {"c", TimingInfo(7, 8, 9)}});
  const std::string r2_string = r2.ToString();
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    xor %%eax,%%eax\n"
      "    cpuid\n"
      ".endr"
      :
      :
      : "%eax", "%ebx", "%ecx", "%edx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Xor) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    xor %%eax,%%eax\n"
      "    xor %%ebx,%%ebx\n"
      "    xor %%ecx,%%ecx\n"
      "    xor %%edx,%%edx\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      "    cvtpd2ps %%xmm1,%%xmm0;  # 3 cycles on port 1, 1 on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    xor %%r11,%%r11\n"
      "    add %%r10,%%r10\n"
      "    add %%r10,%%r11\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtdq2ps %%xmm0,%%xmm1;           # 3 cycles on port 1.\n"
      "    shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtpd2ps %%xmm0,%%xmm1;         # 3 cycles on port 1, 1 on port 5.\n"
      "    shufpd %[shuffle],%%xmm0,%%xmm1; # 1 cycle on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      "    cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsi2sd %%eax,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      "    cvtsd2si %%xmm1,%%eax;  # 3 cycles on port 1, 1 on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsi2sd %%eax,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsi2sdq %%rax,%%xmm1;          # 1 cycle on port 1, 3 on port 5.\n"
      "    shufpd %[shuffle],%%xmm0,%%xmm1; # 1 cycle on port 5.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsi2sd %%rax,%%xmm1 ;  # 1 cycle on port 1, 3 on port 5.\n"
      "    cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.\n"
      ".endr"
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
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.\n"
      "    cvtpd2ps %%xmm0,%%xmm1;  # 3 cycles on port 1, 1 on port 5.\n"
      ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Shufpd) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    shufpd %[shuffle],%%xmm0,%%xmm1;  # 1 cycle on port 5.\n"
      "    shufpd %[shuffle],%%xmm1,%%xmm0;  # 1 cycle on port 5.\n"
      ".endr"
      :
      : [shuffle] "I"(3)));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, MOV64mi32) {
  PerfResult result;
  uint64_t memory;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    movl $123, %[memory]\n"
      ".endr"
      :
      : [memory] "m"(memory)));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, ADDSDrm) {
  PerfResult result;
  double memory;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    addsd %[memory], %%xmm0\n"
      ".endr"
      :
      : [memory] "m"(memory)
      : "%xmm0"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

#ifndef MEMORY_SANITIZER
TEST(PerfSubsystemTest, ADDSDrmSize) {
  double memory;
  asm volatile("movsd %%xmm0,%[memory]" : [ memory ] "=m"(memory) : :);
  EXPECT_NE("", absl::StrFormat("%.17g", memory));
}
#endif

TEST(PerfSubsystemTest, BlockThroughput) {
  // This was extracted from CapProdWithDoubles code.
  PerfResult result;
  uint64_t memory;
  uint64_t address = reinterpret_cast<uint64_t>(&memory);
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    movq %[address], %%rsi\n"
      ".rept 1000\n"
      "    cvtsi2sdq %%rdi, %%xmm2\n"
      "    movsd (%%rsi), %%xmm1\n"
      "    andpd %%xmm1, %%xmm2\n"
      "    movsd (%%rsi),%%xmm0\n"
      "    movaps %%xmm2, %%xmm3\n"
      "    subsd %%xmm0, %%xmm3\n"
      ".endr"
      :
      : [address] "r"(address)
      : "%rsi", "%xmm0", "%xmm1", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJAE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    mov $0xFFFF, %%ecx\n"
      "1:\n"
      "    cvtsi2sdq %%rdi, %%xmm2\n"
      "    cvtsi2sdq %%rsp, %%xmm3\n"
      "    decl %%ecx\n"
      "    cmpl $0x1, %%ecx\n"
      "    jae 1b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJNE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    mov $0xFFFF, %%rcx\n"
      "2:\n"
      "    cvtsi2sdq %%rdi, %%xmm2\n"
      "    cvtsi2sdq %%rsp, %%xmm3\n"
      "    dec %%rcx\n"
      "    jne 2b"\
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJLE) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "2:\n"
      "    cvtsi2sdq %%rdi, %%xmm2\n"
      "    cvtsi2sdq %%rsp, %%xmm3\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jle 2b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, LoopDetectorJL) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "2:\n"
      "    cvtsi2sdq %%rdi, %%xmm2\n"
      "    cvtsi2sdq %%rsp, %%xmm3\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b"
      :
      :
      : "%rcx", "%xmm2", "%xmm3"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// Starting with Sandy Bridge, LEA's with 3 parameters (base, index and offset)
// are executed on port 1 and take as much as 3 cycles.
// The following two benchmarks explore the difference in performance between
// LEA and the corresponding code using ADDs.
TEST(PerfSubsystemTest, Lea) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "    mov $1, %%rax\n"
      "    mov $1, %%rdx\n"
      "2:\n"
      "    lea 2(%%rax, %%rdx, 2), %%rax;  # rax += 2*rdx + 2\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rcx", "%rax", "%rdx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, ReplaceLeaWithAdditions) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx;\n"
      "    mov $1, %%rax\n"
      "    mov $1, %%rdx\n"
      "2:\n"
      "    movq %%rdx, %%rbx;   # rbx = rdx\n"
      "    add $2, %%rax;       # rax += 2\n"
      "    addq %%rdx, %%rbx;   # rbx += rdx ; rbx=2*rdx\n"
      "    addq %%rbx, %%rax;   # rax += rbx ; rax = rax + 2 + 2 * rdx\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rax", "%rbx", "%rcx", "%rdx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// The four benchmarks below explore different ways of expressing
// bool rax = (rcx && rbx).
// - rax is set using setne al / movzx.
// - rax is set using cmovne

TEST(PerfSubsystemTest, TestSetNe) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "    mov $1, %%rbx\n"
      "2:\n"
      "    xor %%eax, %%eax\n"
      "    testq %%rcx, %%rbx\n"
      "    setne %%al;  # al = (rcx && rbx) != 0\n"
      "    ; # optional movzx %%al, %%eax\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rax", "%rbx", "%rcx", "%rdx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, TestSetNeManualRenaming) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "    mov $1, %%rbx\n"
      "2:\n"
      "    movq  %%rcx, %%rdx;  # Rename rcx to rdx & break dependency chain.\n"
      "    xor %%eax, %%eax\n"
      "    testq %%rdx, %%rbx\n"
      "    setne %%al;           # al = (rdx && rbx) != 0\n"
      "    ; # optional movzx %%al, %%eax\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rax", "%rbx", "%rcx", "%rdx"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, TestCmov) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "    mov $1, %%rbx\n"
      "2:\n"
      "    xor %%eax, %%eax\n"
      "    mov $1, %%rdi\n"
      "    test %%rcx, %%rbx\n"
      "    cmovne %%rdi, %%rax;  # al = (rcx && rbx) != 0\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rax", "%rbx", "%rcx", "%rdx", "%rdi"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, TestCmovManualRenaming) {
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      "    xor %%rcx, %%rcx\n"
      "    mov $1, %%rbx\n"
      "2:\n"
      "    movq %%rcx, %%rdx;   # Rename rcx to rdx & break dependency chain.\n"
      "    xor %%eax, %%eax\n"
      "    mov $1, %%rdi\n"
      "    test %%rdx, %%rbx\n"
      "    cmovne %%rdi, %%rax;  # al = (rdx && rbx) != 0\n"
      "    inc %%rcx\n"
      "    cmpq $0xFFFF, %%rcx\n"
      "    jl 2b\n"
      :
      :
      : "%rax", "%rbx", "%rcx", "%rdx", "%rdi"));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, FullCode) {
  PerfResult result;
  int64_t r;
  int64_t size = 8;
  int64_t size_1;
  int64_t log;
  int64_t one;
  int64_t sum;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(
      &result, kIter,
      asm volatile(".rept 1000\n"
                   "     xor %[result], %[result]\n"
                   "     test %[size], %[size]\n"
                   "     je 2\n"
                   "     bsr %[size],%[log]\n"
                   "     leaq -1(%[size]),%[size_1]\n"
                   "     test %[size],%[size_1]\n"
                   "     setne %b[result]\n"
                   "     leaq 2(%[result], %[log], 2), %[result]\n"
                   "2:\n"
                   ".endr\n"
                   : /* output */ [result] "=r"(r), [log] "=&r"(log),
                        [one] "=&r"(one), [sum] "=&r"(sum),
                        [size_1] "=&r"(size_1)
                   : /* input  */ [size] "r"(size)
                   : /* clobbered */));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, FullCodeOptimized) {
  PerfResult result;
  int64_t r;
  int64_t size = 8;
  int64_t size_1;
  int64_t log;
  int64_t sum;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    xor %[result], %[result];    # result = 0\n"
      "    test %[size], %[size];       # if size == 0 return \n"
      "    je 2\n"
      "    bsr %[size],%[log];          # log = ceil(lg2(size))\n"
      "    leaq -1(%[size]),%[size_1];  # size_1 = size - 1\n"
      "    ; # Placing this LEA well in advance shaves .25 c on average.\n"
      "    leaq 2(%[log], %[log]), %[sum];   # sum = 2 + 2 * log\n"
      "    test %[size],%[size_1];      # if ((size - 1) && size)\n"
      "    setne %b[result];            # result = 1\n"
      "    addq %[sum], %[result];      # result += sum\n"
      "2:\n"
      ".endr\n"
       : /* output */ [result] "=r" (r), [log] "=&r"(log),
         [sum] "=&r"(sum), [size_1] "=&r" (size_1)
       : /* input  */ [size] "r" (size)
       : /* clobbered */ ));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsd2ssLatency) {
  // Mesure latency by waiting register cloberring.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsd2ss %%xmm0,%%xmm1\n"
      "    cvtsd2ss %%xmm1,%%xmm0\n"
      ".endr"
      :
      :
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

// The following tests implement the instruction collision mechanism described
// in  go/exegesis:collision .
TEST(PerfSubsystemTest, Cvtsd2ssCollisionOnPort5) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsd2ss %%xmm0,%%xmm1\n"
      "    shufpd %[shuffle],%%xmm0,%%xmm1; # 1 cycle on port 5.\n"
      ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsd2ssCollisionOnPort1) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsd2ss %%xmm0,%%xmm1\n"
      "    cvtdq2ps %%xmm0,%%xmm1;  # 3 cycles on port 1.\n"
      ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsd2ssOverloadingOnPort5) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsd2ss %%xmm0,%%xmm1\n"
      "    shufpd %[shuffle],%%xmm2,%%xmm3; # 1 cycle on port 5.\n"
      ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

TEST(PerfSubsystemTest, Cvtsd2ssOverloadingOnPort1) {
  // Latency.
  PerfResult result;
  // clang-format off
  EXEGESIS_RUN_UNDER_PERF(&result, kIter, asm volatile(
      ".rept 1000\n"
      "    cvtsd2ss %%xmm0,%%xmm1\n"
      "    cvtdq2ps %%xmm2,%%xmm3;  # 3 cycles on port 1.\n"
      ".endr"
      :
      : [shuffle] "I"(3)
      :));
  // clang-format on
  LOG(INFO) << result.ToString();
}

}  // namespace exegesis
