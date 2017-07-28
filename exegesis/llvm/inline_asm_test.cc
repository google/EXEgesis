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

#include "base/stringprintf.h"
#include "exegesis/llvm/llvm_utils.h"
#include "exegesis/util/strings.h"
#include "glog/logging.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

constexpr const char kGenericMcpu[] = "generic";

TEST(JitCompilerTest, CreateAFunctionWithoutLoop) {
  constexpr char kExpectedIR[] =
      "\n"
      "define void @inline_assembly_0() {\n"
      "entry:\n"
      "  call void asm \"mov %ebx, %ecx\", \"~{ebx},~{ecx}\"()\n"
      "  ret void\n"
      "}\n";
  constexpr char kAssemblyCode[] = "mov %ebx, %ecx";
  constexpr char kConstraints[] = "~{ebx},~{ecx}";
  JitCompiler jit(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                  JitCompiler::EXIT_ON_ERROR);
  llvm::Value* const inline_asm =
      jit.AssembleInlineNativeCode(false, kAssemblyCode, kConstraints);
  ASSERT_NE(nullptr, inline_asm);
  llvm::Function* const function =
      jit.WarpInlineAsmInLoopingFunction(1, nullptr, inline_asm, nullptr);
  ASSERT_NE(nullptr, function);
  const std::string function_ir = DumpIRToString(function);
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionWithoutLoopWithInitBlock) {
  constexpr char kExpectedIR[] =
      "\n"
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
  JitCompiler jit(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                  JitCompiler::EXIT_ON_ERROR);
  llvm::Value* const init_inline_asm =
      jit.AssembleInlineNativeCode(false, kInitAssemblyCode, kInitConstraints);
  ASSERT_NE(init_inline_asm, nullptr);
  llvm::Value* const loop_inline_asm =
      jit.AssembleInlineNativeCode(false, kLoopAssemblyCode, kLoopConstraints);
  ASSERT_NE(loop_inline_asm, nullptr);
  llvm::Value* const cleanup_inline_asm = jit.AssembleInlineNativeCode(
      false, kCleanupAssemblyCode, kCleanupConstraints);
  ASSERT_NE(cleanup_inline_asm, nullptr);
  llvm::Function* const function = jit.WarpInlineAsmInLoopingFunction(
      1, init_inline_asm, loop_inline_asm, cleanup_inline_asm);
  ASSERT_NE(function, nullptr);
  const std::string function_ir = DumpIRToString(function);
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionWithLoop) {
  constexpr char kExpectedIR[] =
      "\n"
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
  JitCompiler jit(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                  JitCompiler::EXIT_ON_ERROR);
  llvm::Value* const inline_asm =
      jit.AssembleInlineNativeCode(false, kAssemblyCode, kConstraints);
  ASSERT_NE(nullptr, inline_asm);
  llvm::Function* const function =
      jit.WarpInlineAsmInLoopingFunction(10, nullptr, inline_asm, nullptr);
  ASSERT_NE(nullptr, function);
  const std::string function_ir = DumpIRToString(function);
  EXPECT_EQ(kExpectedIR, function_ir);
}

TEST(JitCompilerTest, CreateAFunctionAndRunItInJIT) {
  constexpr char kAssemblyCode[] = R"(
      .rept 2
      mov %ebx, %eax
      .endr)";
  constexpr char kConstraints[] = "~{ebx},~{eax}";
  JitCompiler jit(llvm::InlineAsm::AD_ATT, kGenericMcpu,
                  JitCompiler::EXIT_ON_ERROR);
  VoidFunction function =
      jit.CompileInlineAssemblyToFunction(10, kAssemblyCode, kConstraints);
  ASSERT_TRUE(function.IsValid());
  // We need to encode at least two two movs (two times 0x89d8).
  const string kTwoMovsEncoding = "\x89\xd8\x89\xd8";
  EXPECT_GE(function.size, kTwoMovsEncoding.size());
  const string compiled_function(reinterpret_cast<const char*>(function.ptr),
                                 function.size);
  LOG(INFO) << "Compiled function: "
            << ToHumanReadableHexString(compiled_function);
  EXPECT_NE(compiled_function.find(kTwoMovsEncoding), string::npos);
  LOG(INFO) << "Calling the function at " << function.ptr;
  function.CallOrDie();
  LOG(INFO) << "Function called";
}

}  // namespace
}  // namespace exegesis
