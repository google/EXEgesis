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

#ifndef EXEGESIS_ITINERARIES_JIT_PERF_EVALUATOR_H_
#define EXEGESIS_ITINERARIES_JIT_PERF_EVALUATOR_H_

#include <string>

#include "exegesis/itineraries/perf_subsystem.h"
#include "exegesis/x86/cpu_state.h"
#include "llvm/IR/InlineAsm.h"
#include "util/task/status.h"

namespace exegesis {

using ::exegesis::util::Status;

// Run Perf on an assembly code string that is to be assembled using the
// LLVM JIT assembler.
// 'dialect' is either llvm::InlineAsm::AD_ATT, or llvm::InlineAsm::INTEL.
// 'measured_code' is repeated 'num_inner_iterations' using .rept/.endr assembly
// directives.
// The results are returned in 'result'.
// The different parameters are used to generate code that will look like:
//     init_code              ; save registers, for example.
//     prefix_code            ; set registers, for example.
// .rept num_inner_iterations
//     measured_code          ; the code that we want to measure.
//     update_code            ; update code (e.g. pointer increment).
// .end
//     suffix_code
//     cleanup_code           ; restore registers, for example.
//
// 'constraints' contains the constraints on the assembly line, in a way similar
// to the inline assembly syntax of gcc or LLVM.
Status EvaluateAssemblyString(
    llvm::InlineAsm::AsmDialect dialect, const std::string& mcpu,
    int num_inner_iterations, const std::string& init_code,
    const std::string& prefix_code, const std::string& measured_code,
    const std::string& update_code, const std::string& suffix_code,
    const std::string& cleanup_code, const std::string& constraints,
    PerfResult* result);

// Executes the given code, measuring the CPU state before and after execution
// of 'code'. 'prefix_code' is run before measurements, and cleanup_code
// afterwards.
Status DebugCPUStateChange(
    llvm::InlineAsm::AsmDialect dialect, const std::string& mcpu,
    const std::string& prefix_code, const std::string& code,
    const std::string& cleanup_code, const std::string& constraints,
    FXStateBuffer* fx_state_buffer_in, FXStateBuffer* fx_state_buffer_out);

}  // namespace exegesis

#endif  // EXEGESIS_ITINERARIES_JIT_PERF_EVALUATOR_H_
