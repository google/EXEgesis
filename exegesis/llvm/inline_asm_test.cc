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

#include "exegesis/llvm/inline_asm.h"

#include "exegesis/llvm/llvm_utils.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/strings.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::HasSubstr;

constexpr const char kGenericMcpu[] = "generic";

TEST(JitCompilerTest, CreateAFunctionWithoutLoop) {
  constexpr char kExpectedIR[] =
      "define void @inline_assembly_0() {\n"
      "entry:\n"
      "  call void asm \"mov %ebx, %ecx\", \"~{ebx},~{ecx}\"()\n"
      "  ret void\n"
      "}\n";
  constexpr char kAssemblyCode[] = "mov %ebx, %ecx";
  constexpr char kConstraints[] = "~{ebx},~{ecx}";
  JitCompiler jit(kGenericMcpu);
  llvm::Value* const inline_asm = jit.AssembleInlineNativeCode(
      false, kAssemblyCode, kConstraints, llvm::InlineAsm::AD_ATT);
  ASSERT_NE(nullptr, inline_asm);
  const auto function =
      jit.WrapInlineAsmInLoopingFunction(1, nullptr, inline_asm, nullptr);
  ASSERT_OK(function);
  const std::string function_ir = DumpIRToString(function.ValueOrDie());
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionWithoutLoopWithInitBlock) {
  constexpr char kExpectedIR[] =
      "define void @inline_assembly_0() {\n"
      "entry:\n"
      "  call void asm \"mov %ebx, 0x1234\", \"~{ebx}\"()\n"
      "  call void asm \"mov %ecx, %ebx\", \"~{ebx},~{ecx}\"()\n"
      "  call void asm \"mov %edx, 0x5678\", \"~{edx}\"()\n"
      "  ret void\n"
      "}\n";
  constexpr char kInitAssemblyCode[] = "mov %ebx, 0x1234";
  constexpr char kInitConstraints[] = "~{ebx}";
  constexpr char kLoopAssemblyCode[] = "mov %ecx, %ebx";
  constexpr char kLoopConstraints[] = "~{ebx},~{ecx}";
  constexpr char kCleanupAssemblyCode[] = "mov %edx, 0x5678";
  constexpr char kCleanupConstraints[] = "~{edx}";
  JitCompiler jit(kGenericMcpu);
  llvm::Value* const init_inline_asm = jit.AssembleInlineNativeCode(
      false, kInitAssemblyCode, kInitConstraints, llvm::InlineAsm::AD_ATT);
  ASSERT_NE(init_inline_asm, nullptr);
  llvm::Value* const loop_inline_asm = jit.AssembleInlineNativeCode(
      false, kLoopAssemblyCode, kLoopConstraints, llvm::InlineAsm::AD_ATT);
  ASSERT_NE(loop_inline_asm, nullptr);
  llvm::Value* const cleanup_inline_asm = jit.AssembleInlineNativeCode(
      false, kCleanupAssemblyCode, kCleanupConstraints,
      llvm::InlineAsm::AD_ATT);
  ASSERT_NE(cleanup_inline_asm, nullptr);
  const auto function = jit.WrapInlineAsmInLoopingFunction(
      1, init_inline_asm, loop_inline_asm, cleanup_inline_asm);
  ASSERT_OK(function);
  const std::string function_ir = DumpIRToString(function.ValueOrDie());
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionWithLoop) {
  constexpr char kExpectedIR[] =
      "define void @inline_assembly_0() {\n"
      "entry:\n"
      "  br label %loop\n"
      "\n"
      "loop:                                             ; preds = %loop, "
      "%entry\n"
      "  %counter = phi i32 [ 10, %entry ], [ %new_counter, %loop ]\n"
      "  call void asm \"mov %ebx, %ecx\", \"~{ebx},~{ecx}\"()\n"
      "  %new_counter = sub i32 %counter, 1\n"
      "  %0 = icmp sgt i32 %new_counter, 0\n"
      "  br i1 %0, label %loop, label %loop_end\n"
      "\n"
      "loop_end:                                         ; preds = %loop\n"
      "  ret void\n"
      "}\n";
  constexpr char kAssemblyCode[] = "mov %ebx, %ecx";
  constexpr char kConstraints[] = "~{ebx},~{ecx}";
  JitCompiler jit(kGenericMcpu);
  llvm::Value* const inline_asm = jit.AssembleInlineNativeCode(
      false, kAssemblyCode, kConstraints, llvm::InlineAsm::AD_ATT);
  ASSERT_NE(nullptr, inline_asm);
  const auto function =
      jit.WrapInlineAsmInLoopingFunction(10, nullptr, inline_asm, nullptr);
  ASSERT_OK(function);
  const std::string function_ir = DumpIRToString(function.ValueOrDie());
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionAndRunItInJIT) {
  constexpr char kAssemblyCode[] = R"(
      .rept 2
      mov %ebx, %eax
      .endr)";
  constexpr char kConstraints[] = "~{ebx},~{eax}";
  JitCompiler jit(kGenericMcpu);
  const auto function = jit.CompileInlineAssemblyToFunction(
      10, kAssemblyCode, kConstraints, llvm::InlineAsm::AD_ATT);
  ASSERT_OK(function);
  // We need to encode at least two two movs (two times 0x89d8).
  const std::string kTwoMovsEncoding = "\x89\xd8\x89\xd8";
  EXPECT_GE(function.ValueOrDie().size, kTwoMovsEncoding.size());
  const std::string compiled_function(
      reinterpret_cast<const char*>(function.ValueOrDie().ptr),
      function.ValueOrDie().size);
  LOG(INFO) << "Compiled function: "
            << ToHumanReadableHexString(compiled_function);
  EXPECT_THAT(compiled_function, HasSubstr(kTwoMovsEncoding));
  LOG(INFO) << "Calling the function at " << function.ValueOrDie().ptr;
  function.ValueOrDie().CallOrDie();
  LOG(INFO) << "Function called";
}

TEST(JitCompilerTest, UnknownReferencedSymbols) {
  JitCompiler jit(kGenericMcpu);
  const auto function = jit.CompileInlineAssemblyToFunction(
      2, R"(mov ebx, unknown_symbol)", "", llvm::InlineAsm::AD_Intel);

  ASSERT_THAT(
      function.status(),
      StatusIs(
          INVALID_ARGUMENT,
          "The following unknown symbols are referenced: 'unknown_symbol'"));
}

}  // namespace
}  // namespace exegesis
