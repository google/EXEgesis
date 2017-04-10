#include "cpu_instructions/itineraries/jit_perf_evaluator.h"

#include <cstdint>

#include "base/stringprintf.h"
#include "cpu_instructions/itineraries/perf_subsystem.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace cpu_instructions {
namespace {

using ::testing::HasSubstr;

const int kOuterIter = 1000;
const int kInnerIter = 1024;

constexpr const char kGenericMcpu[] = "generic";

void TestEvaluateAssemblyString(const std::string& measured_code,
                                const std::string& constraints) {
  PerfResult result;
  ASSERT_OK(EvaluateAssemblyString(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                                   kOuterIter, kInnerIter,
                                   /*init_code=*/"",
                                   /*prefix_code=*/"", measured_code,
                                   /*update_code=*/"",
                                   /*suffix_code=*/"",
                                   /*cleanup_code=*/"", constraints, &result));
  const string result_string = PerfResultString(result);
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
      llvm::InlineAsm::AD_Intel, kGenericMcpu, kOuterIter, kInnerIter,
      /*init_code=*/StringPrintf("movabs r11,%p", &memory),
      /*prefix_code=*/"",
      /*measured_code=*/"addsd xmm0,qword ptr [r11]",
      /*update_code=*/"",
      /*suffix_code=*/"",
      /*cleanup_code=*/"",
      /*constraints=*/"~{r11},~{xmm0}", &result));
  const string result_string = PerfResultString(result);
  EXPECT_THAT(result_string, HasSubstr("num_times"));
  LOG(INFO) << result_string;
}

#ifndef MEMORY_SANITIZER
TEST(JitPerfEvaluatorTest, Mov64mi32ATT) {
  int64_t memory;
  PerfResult result;
  ASSERT_OK(EvaluateAssemblyString(
      llvm::InlineAsm::AD_ATT, kGenericMcpu, kOuterIter, kInnerIter,
      /*init_code=*/"",
      /*prefix_code=*/StringPrintf("movabsq $$%p,%%r11", &memory),
      /*measured_code=*/"movq $$64,(%r11)",
      /*update_code=*/"",
      /*suffix_code=*/"",
      /*cleanup_code=*/"",
      /*constraints=*/"~{r11}", &result));
  DCHECK_EQ(64, memory);
  const string result_string = PerfResultString(result);
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
  const string prefix_code = StringPrintf(
      R"(
        movabs rsi,%p
        fstcw word ptr[rsi]
      )",
      &fpu_control_word_save);

  // Load control word from fpu_control_word_out.
  const string code = StringPrintf(
      R"(
        movabs rdi,%p
        fldcw word ptr[rdi]
      )",
      &fpu_control_word_out);

  // Restore previous control word.
  const string cleanup_code =
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
}  // namespace cpu_instructions
