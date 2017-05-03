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

#include "cpu_instructions/itineraries/jit_perf_evaluator.h"

#include <cstdint>
#include <map>
#include <vector>
#include "strings/string.h"

#include "base/stringprintf.h"
#include "cpu_instructions/itineraries/perf_subsystem.h"
#include "cpu_instructions/llvm/inline_asm.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace cpu_instructions {

namespace {
std::string RepeatCode(int num_repeats, const std::string& code) {
  return StrCat(".rept ", num_repeats, "\n", code, "\n.endr\n");
}

using util::OkStatus;

}  // namespace

// The list of perf events we want to measure.
constexpr const PerfSubsystem::EventCategory kPerfEventCategories[] = {
    &PerfEventsProto::cycle_events, &PerfEventsProto::computation_events,
    &PerfEventsProto::memory_events, &PerfEventsProto::uops_events};

Status EvaluateAssemblyString(
    llvm::InlineAsm::AsmDialect dialect, const string& mcpu,
    const int num_outer_iterations, const int num_inner_iterations,
    const std::string& init_code, const std::string& prefix_code,
    const std::string& measured_code, const std::string& update_code,
    const std::string& suffix_code, const std::string& cleanup_code,
    const std::string& constraints, PerfResult* result) {
  JitCompiler jit(dialect, mcpu, JitCompiler::RETURN_NULLPTR_ON_ERROR);
  const string code =
      StrCat(prefix_code, "\n",
             RepeatCode(num_inner_iterations,
                        StrCat(measured_code, "\n\t", update_code)),
             "\n", suffix_code);
  // NOTE(bdb): constraints are the same for 'code', 'init_code' and
  // 'cleanup_code'.
  VoidFunction inline_asm_function = jit.CompileInlineAssemblyToFunction(
      num_outer_iterations, init_code, constraints, code, constraints,
      cleanup_code, constraints);
  if (!inline_asm_function.IsValid()) {
    return util::UnknownError("Could not compile the measured code");
  }

  // Because of the decode window size, a large instruction is likely going to
  // take at least 1 cycle on average.
  // Make sure that the repeated instruction fits in the cache to avoid noise
  // from cache misses.
  constexpr int kL1CodeCacheSize = 1 << 15;
  if (inline_asm_function.size >= kL1CodeCacheSize) {
    return util::UnknownError(
        StrCat("Cannot fit ", num_inner_iterations,
               " repetitions of the measured code in the L1 cache"));
  }

  PerfSubsystem perf_subsystem;
  for (const auto& events : kPerfEventCategories) {
    perf_subsystem.StartCollectingEvents(events);
    inline_asm_function.CallOrDie();
    perf_subsystem.StopAndReadCounters(result);
  }
  result->SetScaleFactor(num_outer_iterations * num_inner_iterations);
  return OkStatus();
}

Status DebugCPUStateChange(llvm::InlineAsm::AsmDialect dialect,
                           const string& mcpu, const std::string& prefix_code,
                           const std::string& code,
                           const std::string& cleanup_code,
                           const std::string& constraints,
                           FXStateBuffer* fx_state_buffer_in,
                           FXStateBuffer* fx_state_buffer_out) {
  JitCompiler jit(dialect, mcpu, JitCompiler::RETURN_NULLPTR_ON_ERROR);

  constexpr const char kGetStateCodeTemplate[] = R"(
    push rax
    movabs rax,%p
    fxsave64 opaque ptr [rax]
    pop rax
  )";

  const string in_code =
      StringPrintf(kGetStateCodeTemplate, fx_state_buffer_in->get());

  const string out_code =
      StringPrintf(kGetStateCodeTemplate, fx_state_buffer_out->get());

  const VoidFunction inline_asm_function = jit.CompileInlineAssemblyToFunction(
      /*num_iterations=*/1,
      StrCat(prefix_code, in_code, code, out_code, cleanup_code), constraints);
  if (!inline_asm_function.IsValid()) {
    return util::UnknownError("Could not compile the assembly code");
  }
  inline_asm_function.CallOrDie();

  return OkStatus();
}

}  // namespace cpu_instructions
