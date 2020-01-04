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

#include "exegesis/itineraries/jit_perf_evaluator.h"

#include <cstdint>

#include "absl/strings/str_format.h"
#include "exegesis/itineraries/perf_subsystem.h"
#include "exegesis/testing/test_util.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::testing::HasSubstr;

const int kInnerIter = 1024;

constexpr const char kGenericMcpu[] = "generic";

void TestEvaluateAssemblyString(const std::string& measured_code,
                                const std::string& constraints) {
  PerfResult result;
  ASSERT_OK(EvaluateAssemblyString(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                                   kInnerIter,
                                   /*init_code=*/"",
                                   /*prefix_code=*/"", measured_code,
                                   /*update_code=*/"",
                                   /*suffix_code=*/"",
                                   /*cleanup_code=*/"", constraints, &result));
  const std::string result_string = result.ToString();
  EXPECT_THAT(result_string, HasSubstr("num_times"));
  LOG(INFO) << result_string;
}

TEST(JitPerfEvaluatorTest, Mov) {
  TestEvaluateAssemblyString(
      R"(
        movl %eax, %edx
        movl %ecx, %ebx
      )",
      "~{eax},~{edx},~{ecx},~{ebx}");
}

TEST(JitPerfEvaluatorTest, AddEcxToEdx) {
  TestEvaluateAssemblyString("addl %ecx, %edx", "~{ecx},~{edx}");
}

TEST(JitPerfEvaluatorTest, ComputeInt64Max) {
  TestEvaluateAssemblyString(
      R"(
        xorq %rdx,%rdx
        notq %rdx
        shrq $$1, %rdx
      )",
      "~{rdx}");
}

TEST(JitPerfEvaluatorTest, MovInt64) {
  TestEvaluateAssemblyString("movabsq $$5124095575370701, %r11", "~{r11}");
}

TEST(JitPerfEvaluatorTest, Add64ri8) {
  TestEvaluateAssemblyString(
      R"(
        addq $$15,%rax
        addq $$16,%rbx
      )",
      "~{rax},~{rbx}");
}

TEST(JitPerfEvaluatorTest, AddsdrmIntel) {
  double memory[10];
  PerfResult result;
  ASSERT_OK(EvaluateAssemblyString(
      llvm::InlineAsm::AD_Intel, kGenericMcpu, kInnerIter,
      /*init_code=*/absl::StrFormat("movabs r11,%p", &memory),
      /*prefix_code=*/"",
      /*measured_code=*/"addsd xmm0,qword ptr [r11]",
      /*update_code=*/"",
      /*suffix_code=*/"",
      /*cleanup_code=*/"",
      /*constraints=*/"~{r11},~{xmm0}", &result));
  const std::string result_string = result.ToString();
  EXPECT_THAT(result_string, HasSubstr("num_times"));
  LOG(INFO) << result_string;
}

#ifndef MEMORY_SANITIZER
TEST(JitPerfEvaluatorTest, Mov64mi32ATT) {
  int64_t memory;
  PerfResult result;
  ASSERT_OK(EvaluateAssemblyString(
      llvm::InlineAsm::AD_ATT, kGenericMcpu, kInnerIter,
      /*init_code=*/"",
      /*prefix_code=*/absl::StrFormat("movabsq $$%p,%%r11", &memory),
      /*measured_code=*/"movq $$64,(%r11)",
      /*update_code=*/"",
      /*suffix_code=*/"",
      /*cleanup_code=*/"",
      /*constraints=*/"~{r11}", &result));
  DCHECK_EQ(64, memory);
  const std::string result_string = result.ToString();
  EXPECT_THAT(result_string, HasSubstr("num_times"));
  LOG(INFO) << result_string;
}
#endif

TEST(JitPerfEvaluatorTest, CvtSi2Sd) {
  TestEvaluateAssemblyString(
      /*measured_code=*/"cvtsi2sd %edx,%xmm0",
      /*constraints=*/"~{xmm0}");
}

TEST(JitPerfEvaluatorTest, DebugCPUStateChange) {
  constexpr const uint64_t kExpectedFPUControlWord = 0x0025;
  uint16_t fpu_control_word_save = 0;
  // Single precision, nearest, exceptions: Invalid Op, Zero Divide, Precision.
  uint16_t fpu_control_word_out = kExpectedFPUControlWord;

  // Save previous control word.
  const std::string prefix_code = absl::StrFormat(
      R"(
        movabs rsi,%p
        fstcw word ptr[rsi]
      )",
      &fpu_control_word_save);

  // Load control word from fpu_control_word_out.
  const std::string code = absl::StrFormat(
      R"(
        movabs rdi,%p
        fldcw word ptr[rdi]
      )",
      &fpu_control_word_out);

  // Restore previous control word.
  const std::string cleanup_code =
      R"(
        fldcw word ptr[rsi]
      )";

  // Checks that we setting the control word is correctly measured.
  FXStateBuffer fx_state_buffer_in;
  FXStateBuffer fx_state_buffer_out;
  EXPECT_OK(DebugCPUStateChange(llvm::InlineAsm::AD_Intel, kGenericMcpu,
                                prefix_code, code, cleanup_code,
                                /*constraints=*/"~{rsi},~{rdi}",
                                &fx_state_buffer_in, &fx_state_buffer_out));
  LOG(INFO) << fx_state_buffer_in.DebugString();
  LOG(INFO) << fx_state_buffer_out.DebugString();
  constexpr const uint16_t kMaskOutReserved = 0x1f3f;
  EXPECT_EQ(
      fpu_control_word_save & kMaskOutReserved,
      fx_state_buffer_in.GetFPUControlWord().raw_value_ & kMaskOutReserved);
  EXPECT_EQ(
      kExpectedFPUControlWord,
      fx_state_buffer_out.GetFPUControlWord().raw_value_ & kMaskOutReserved);
}

}  // namespace
}  // namespace exegesis
