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
#include <map>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "exegesis/itineraries/perf_subsystem.h"
#include "exegesis/llvm/inline_asm.h"
#include "exegesis/util/strings.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace {

using util::OkStatus;

std::string RepeatCode(int num_repeats, const std::string& code) {
  return absl::StrCat(".rept ", num_repeats, "\n", code, "\n.endr\n");
}

}  // namespace

// The list of perf events we want to measure.
constexpr const PerfSubsystem::EventCategory kPerfEventCategories[] = {
    &PerfEventsProto::cycle_events, &PerfEventsProto::computation_events,
    &PerfEventsProto::memory_events, &PerfEventsProto::uops_events};

Status EvaluateAssemblyString(
    llvm::InlineAsm::AsmDialect dialect, const std::string& mcpu,
    const int num_inner_iterations, const std::string& init_code,
    const std::string& prefix_code, const std::string& measured_code,
    const std::string& update_code, const std::string& suffix_code,
    const std::string& cleanup_code, const std::string& constraints,
    PerfResult* result) {
  JitCompiler jit(mcpu);
  const std::string code =
      absl::StrCat(prefix_code, "\n",
                   RepeatCode(num_inner_iterations,
                              absl::StrCat(measured_code, "\n\t", update_code)),
                   "\n", suffix_code);
  // NOTE(bdb): constraints are the same for 'code', 'init_code' and
  // 'cleanup_code'.
  const auto inline_asm_function = jit.CompileInlineAssemblyToFunction(
      1, init_code, constraints, code, constraints, cleanup_code, constraints,
      dialect);
  if (!inline_asm_function.ok()) {
    return util::UnknownError(absl::StrCat(
        "Could not compile the measured code:",
        ToStringView(inline_asm_function.status().error_message())));
  }

  PerfSubsystem perf_subsystem;
  for (const auto& events : kPerfEventCategories) {
    perf_subsystem.StartCollectingEvents(events);
    inline_asm_function.ValueOrDie().CallOrDie();
    result->Accumulate(perf_subsystem.StopAndReadCounters());
  }
  result->SetScaleFactor(num_inner_iterations);
  return OkStatus();
}

Status DebugCPUStateChange(
    llvm::InlineAsm::AsmDialect dialect, const std::string& mcpu,
    const std::string& prefix_code, const std::string& code,
    const std::string& cleanup_code, const std::string& constraints,
    FXStateBuffer* fx_state_buffer_in, FXStateBuffer* fx_state_buffer_out) {
  JitCompiler jit(mcpu);

  constexpr const char kGetStateCodeTemplate[] = R"(
    push rax
    movabs rax,%p
    fxsave64 [rax]
    pop rax
  )";

  const std::string in_code =
      absl::StrFormat(kGetStateCodeTemplate, fx_state_buffer_in->get());

  const std::string out_code =
      absl::StrFormat(kGetStateCodeTemplate, fx_state_buffer_out->get());

  const auto inline_asm_function = jit.CompileInlineAssemblyToFunction(
      /*num_iterations=*/1,
      absl::StrCat(prefix_code, in_code, code, out_code, cleanup_code),
      constraints, dialect);
  if (!inline_asm_function.ok()) {
    return inline_asm_function.status();
  }
  inline_asm_function.ValueOrDie().CallOrDie();

  return OkStatus();
}

}  // namespace exegesis
