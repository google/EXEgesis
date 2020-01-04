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

#include "exegesis/itineraries/compute_itineraries.h"

#include <stddef.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "base/commandlineflags.h"
#include "exegesis/base/cpu_info.h"
#include "exegesis/base/host_cpu.h"
#include "exegesis/base/prettyprint.h"
#include "exegesis/itineraries/decomposition.h"
#include "exegesis/itineraries/jit_perf_evaluator.h"
#include "exegesis/itineraries/perf_subsystem.h"
#include "exegesis/llvm/inline_asm.h"
#include "exegesis/util/category_util.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/x86/cpu_state.h"
#include "exegesis/x86/operand_translator.h"
#include "glog/logging.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Support/Host.h"
#include "net/proto2/util/public/repeated_field_util.h"
#include "re2/re2.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/text_format.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace itineraries {
namespace {

using ::exegesis::util::InternalError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

const absl::flat_hash_set<std::string>* const kExcludedInstructions =
    new absl::flat_hash_set<std::string>({
        // Before execution, "DX:AX" == 0x00010001 and
        // "word ptr[RSI]" == 0x0001, so 0x00010001/0x0001 == 0x00010001
        // overflows, resulting in #DE.
        "DIV",
        "IDIV",
        // Interrupt-related.
        "INT3",
        "INT",
        "IRET",
        "IRETD",
        "IRETQ",
        // This tries to read FPU state from RSI, which does not have the right
        // structure. This would require the contents of RSI to be properly
        // structured.
        "FLDENV",
        "FLDCW",
        "FXRSTOR",
        "FXRSTOR64",
        // This tries to set reserved bits to 1 ("Bits 16 through 31 of the
        // MXCSR register are reserved and are cleared on a power-up or reset of
        // the processor;attempting to write a non-zero value to these bits,
        // using either the FXRSTOR or LDMXCSR instructions, will result in a
        // general-protection exception (#GP) being generated.")
        "LDMXCSR",
        "VLDMXCSR",
        // The value loaded in RSI correspond to an invalid descriptor (null) or
        // not within writable bounds, and thus triggers a #GP.
        "LFS",
        "LGS",
        "LSL",
        "LSS",
        // LOCK requires and accompanying instruction.
        "LOCK",
        // #GP because "the value in EAX is outside the CS, DS, ES, FS, or GS
        // segment limit".
        "MONITOR",
        // Stack instructions. Obviously running a million POPs is a bad idea.
        "POP",
        "POPF",
        "POPFQ",
        "PUSH",
        "PUSHF",
        "PUSHFQ",
        // This cannot be tested (by design).
        "UD2",
        // These require memory to be aligned more than 16 bytes.
        "VMOVAPD",
        "VMOVAPS",
        "VMOVDQA",
        "VMOVNTDQ",
        "VMOVNTDQA",
        "VMOVNTPD",
        "VMOVNTPS",
        // These cannot be called several times successively.
        "VPGATHERDD",
        "VGATHERDPS",
        // This would require ECX to be 0 instead of 1: "XCR0 is supported on
        // any processor that supports the XGETBV instruction."
        "XGETBV",
        "XSETBV",
        // This tries to read extended registers state from RSI, which does not
        // have the right structure. This would require the contents of RSI to
        // be properly structured.
        "XRSTOR",
        "XRSTOR64",
        // These require memory to be 64-byte aligned and EDX:EAX to be set to
        // specific values.
        "XSAVE",
        "XSAVE64",
        "XSAVEC",
        "XSAVEC64",
        "XSAVEOPT",
        "XSAVEOPT64",
        "XSAVES",
        "XSAVES64",

        // Sys instructions.
        "SYSCALL",
        "SYSENTER",
        "SYSEXIT",
        "SYSRET",
        // Program flow.
        "CALL",
        "JMP",
        "ENTER",
        "LEAVE",
        "RET",
        // Conditional jumps.
        "JA",
        "JAE",
        "JB",
        "JBE",
        "JC",
        "JE",
        "JG",
        "JGE",
        "JL",
        "JLE",
        "JNA",
        "JNAE",
        "JNB",
        "JNBE",
        "JNC",
        "JNE",
        "JNG",
        "JNGE",
        "JNL",
        "JNLE",
        "JNO",
        "JNP",
        "JNS",
        "JNZ",
        "JO",
        "JP",
        "JPE",
        "JPO",
        "JS",
        "JZ",
        "JCXZ",
        "JECXZ",
        "JRCXZ",
        // LOOP.
        "LOOP",
        "LOOPE",
        "LOOPNE",
    });

ObservationVector CreateObservationVector(const PerfResult& perf_result) {
  ObservationVector observations;

  bool at_least_one_non_zero = false;
  for (const auto& name : perf_result.Keys()) {
    // Make all the events look like Haswell events.
    // TODO(bdb): This should depend on CPUInfo.
    std::string key = name;
    absl::StrReplaceAll(
        {{"uops_dispatched_port:port_", "uops_executed_port:port_"},
         {"uops_executed:port", "uops_executed_port:port_"},
         {"uops_executed.all", "uops_executed"}},
        &key);
    ObservationVector::Observation* const observation =
        observations.add_observations();
    observation->set_event_name(key);
    const double measurement = perf_result.GetScaledOrDie(name);
    CHECK_GE(measurement, 0.0);
    observation->set_measurement(measurement);
    at_least_one_non_zero = at_least_one_non_zero || measurement != 0.0;
  }
  CHECK(at_least_one_non_zero);
  return observations;
}

using PortMaskCount = absl::flat_hash_map<PortMask, int, PortMask::Hash>;

// A helper to compute itineraries. Every instruction is measured by generating
// example code for the instruction, which is essentially the instruction
// repeated `inner_iterations` times (to handle instructions that read or write
// to memory, we insert additional update code, see notes on UpdateCode below).
// To get significant measurements, this inner block is evaluated in a loop with
// `outer_iterations` iterations.
class ComputeItinerariesHelper {
 public:
  struct Parameters {
    // The total size needed for buffers used by instructions that read from or
    // write to memory. RSI is increased by rsi_step at every iteration, and
    // each instruction can read up to max_bytes_touched_per_instruction.
    int GetBufferSize() const {
      return inner_iterations * rsi_step + max_bytes_touched_per_instruction;
    }

    // Number of times an instruction is repeated in the loop body.
    int inner_iterations = 10000;
    // The number of bytes to increase RSI by after each instruction block. We
    // chose 16 as the increment as some vector instructions need to be aligned
    // that way.
    int rsi_step = 16;
    // The maximum number of bytes touched by any single instruction.
    int max_bytes_touched_per_instruction = 512;
  };

  ComputeItinerariesHelper(const CpuInfo& cpu_info,
                           const MicroArchitecture& microarchitecture,
                           const Parameters& parameters);

  Status ComputeItineraries(const InstructionSetProto& instruction_set,
                            InstructionSetItinerariesProto* itineraries) const;

 private:
  class Stats {
   public:
    static constexpr const size_t kNumQuantiles = 100;
    static constexpr const size_t kMaxNumUops = 20;

    using Histogram = std::array<int, kMaxNumUops>;

    Stats() {
      for (Histogram& histogram : uop_stats_) {
        histogram.fill(0);
      }
    }

    void IncrementAssemblyErrors() {
      ++num_instructions_;
      ++num_assembly_errors_;
    }

    void IncrementDecodeStallsErrors() {
      ++num_instructions_;
      ++num_decode_stalls_;
    }

    void IncrementUnsolvedProblems() {
      ++num_instructions_;
      ++num_unsolved_mips_;
      // Count this as the maximum error with 0 instructions.
      ++uop_stats_[kNumQuantiles - 1][0];
    }

    void IncrementSolvedProblems(const DecompositionSolver& solver) {
      ++num_instructions_;
      const int quantile =
          static_cast<int>(solver.max_error_value() * kNumQuantiles);
      const size_t num_uops =
          std::min(kMaxNumUops - 1, solver.port_loads().size());
      ++uop_stats_[quantile][num_uops];
      if (solver.is_order_unique()) {
        ++num_instructions_with_unique_order_;
      }
    }

    void IncrementSubtractUpdateCodeErrors() {
      ++num_subtract_update_code_errors_;
    }

    std::string DebugString() const {
      std::string result;
      for (int quantile = 0; quantile < kNumQuantiles; ++quantile) {
        const double q = static_cast<double>(quantile);
        absl::StrAppendFormat(&result, "[%.02f %.02f), ", q / kNumQuantiles,
                              (q + 1) / kNumQuantiles);
        absl::StrAppend(&result, absl::StrJoin(uop_stats_[quantile], ", "),
                        "\n");
      }

      absl::StrAppend(&result, num_unsolved_mips_, " unsolved MIPs.\n");
      absl::StrAppend(&result, num_instructions_with_unique_order_,
                      " unique orders.\n");
      absl::StrAppend(&result, num_instructions_, " instructions.\n");
      absl::StrAppend(&result, num_assembly_errors_, " assembly errors.\n");
      absl::StrAppend(&result, num_decode_stalls_,
                      " stalled decode pipeline errors.\n");
      absl::StrAppend(&result, num_subtract_update_code_errors_,
                      " subtract update code errors.\n");
      return result;
    }

   private:
    int num_instructions_ = 0;
    int num_unsolved_mips_ = 0;
    int num_assembly_errors_ = 0;
    int num_decode_stalls_ = 0;
    int num_instructions_with_unique_order_ = 0;
    int num_subtract_update_code_errors_ = 0;
    std::array<Histogram, kNumQuantiles> uop_stats_;
  };

  // Returns the initialization code.
  static std::string MakeInitCode(uint8* fx_state_buffer);

  // Code to setup runtime environment to ensure that instructions execute
  // "normally" (e.g. setup operands to avoid division by zero).
  static std::string MakePrefixCode(char* src_buffer, char* dst_buffer);
  // Update code run between every instruction execution.
  static std::string MakeUpdateCode(int rsi_step);

  // Returns the cleanup code.
  static std::string MakeCleanupCode(uint8* fx_state_buffer);

  // Computes the itineraries for the update code.
  StatusOr<PortMaskCount> ComputeUpdateCodeMicroOps() const;

  Status ComputeOneItinerary(const InstructionProto& instruction,
                             const PortMaskCount& update_code_micro_ops,
                             ItineraryProto* const itinerary,
                             Stats* const stats) const;

  const MicroArchitecture& microarchitecture_;
  const CpuInfo& cpu_info_;
  const std::string host_mcpu_;
  const Parameters parameters_;
  // Source and destination buffers for instructions that read from or write to
  // memory.
  std::unique_ptr<char[]> src_buffer_;
  std::unique_ptr<char[]> dst_buffer_;
  // A buffer for saving and restoring the FPU state.
  FXStateBuffer fx_state_buffer_;
  const std::string init_code_;
  const std::string prefix_code_;
  const std::string update_code_;
  const std::string cleanup_code_;
  const std::string constraints_;
};

ComputeItinerariesHelper::ComputeItinerariesHelper(
    const CpuInfo& cpu_info, const MicroArchitecture& microarchitecture,
    const Parameters& parameters)
    : microarchitecture_(microarchitecture),
      cpu_info_(cpu_info),
      host_mcpu_(::llvm::sys::getHostCPUName().str()),
      parameters_(parameters),
      src_buffer_(new char[parameters_.GetBufferSize()]),
      dst_buffer_(new char[parameters_.GetBufferSize()]),
      init_code_(MakeInitCode(fx_state_buffer_.get())),
      prefix_code_(MakePrefixCode(src_buffer_.get(), dst_buffer_.get())),
      update_code_(MakeUpdateCode(parameters_.rsi_step)),
      cleanup_code_(MakeCleanupCode(fx_state_buffer_.get())),
      // It's super-important that the registers used in the benchmark code be
      // referenced as overwritten in the constraints string. The measurements
      // may otherwise be wrong.
      // TODO(courbet): Generate the constraints automatically.
      constraints_(
          "~{rax},~{rbx},~{rcx},~{rdx},~{rsi},~{rdi},~{mm6},~{xmm1},~{xmm5},"
          "~{r8},~{r9},~{r10}") {
  LOG(INFO) << "Host MCPU is '" << host_mcpu_ << "'";
  // Initialize the memory read buffer with valid values.
  std::fill(src_buffer_.get(), src_buffer_.get() + parameters_.GetBufferSize(),
            1);
}

// Note that LLVM's inline assembly does not understand MOV r,imm64
// in the Intel mode. We have to use movabs instead.
std::string ComputeItinerariesHelper::MakeInitCode(
    uint8* const fx_state_buffer) {
  // Store the FPU/MMX/SSE state. We'll reinstate it after the code under
  // test. This is to ensure that there is no contamination between
  // measurements.
  return absl::StrFormat(
      R"(
        movabs rax,%p
        fxsave64 [rax]
      )",
      fx_state_buffer);
}

std::string ComputeItinerariesHelper::MakePrefixCode(char* const src_buffer,
                                                     char* const dst_buffer) {
  return absl::StrFormat(
      R"(
        # Load constants into registers to not break instructions like
        # BT or FP instructions.
        fld1
        mov eax,1
        mov ecx,1
        mov edx,1
        mov r8,1
        mov r9,1
        mov r10,1
        # Set RSI = &src_buffer;
        movabs rsi,%p
        # Set RDI = &dst_buffer;
        movabs rdi,%p
      )",
      src_buffer, dst_buffer);
}

// NOTE(bdb): If we do not increment the value of RSI (which is also used as
// the destination register) the instructions that write to memory always do
// it at the same location. This in turn results in performance measurements
// that are difficult, if at all possible, to understand, with a lot of
// data_write micro-operations.
// TODO(bdb): Use RDI as the destination register. Increment only when
// memory is written to.
std::string ComputeItinerariesHelper::MakeUpdateCode(const int rsi_step) {
  return absl::StrFormat(
      R"(
        ADD RSI,%i
      )",
      rsi_step);
}

std::string ComputeItinerariesHelper::MakeCleanupCode(
    uint8* const fx_state_buffer) {
  return absl::StrFormat(
      R"(
        # Restore FPU/MMX/SSE state.
        movabs rax,%p
        fxrstor64 [rax]
      )",
      fx_state_buffer);
}

// Returns true if an intruction reads from or writes to memory.
// TODO(courbet): We actually only care about instructions that *write* to
// memory. However for now this information is not present on all instructions.
// We should revisit that when it's the case.
bool TouchesMemory(const InstructionFormat& asm_syntax) {
  for (const InstructionOperand& operand : asm_syntax.operands()) {
    CHECK_NE(operand.addressing_mode(),
             InstructionOperand::ANY_ADDRESSING_MODE);
    if (InCategory(operand.addressing_mode(),
                   InstructionOperand::INDIRECT_ADDRESSING) ||
        InCategory(operand.addressing_mode(),
                   InstructionOperand::ANY_ADDRESSING_WITH_FIXED_REGISTERS)) {
      return true;
    }
  }
  return false;
}

StatusOr<PortMaskCount> ComputeItinerariesHelper::ComputeUpdateCodeMicroOps()
    const {
  PerfResult result;
  CHECK_OK(EvaluateAssemblyString(
      llvm::InlineAsm::AD_Intel, host_mcpu_, parameters_.inner_iterations,
      init_code_, prefix_code_,
      /*measured_code=*/"", update_code_,
      /*suffix_code=*/"", cleanup_code_, constraints_, &result));
  const ObservationVector observation = CreateObservationVector(result);
  DecompositionSolver solver(microarchitecture_);
  const Status status = solver.Run(observation);
  if (!status.ok()) {
    return status;
  }
  const auto micro_ops = solver.GetMicroOps();
  PortMaskCount port_masks;
  for (const MicroOperationProto& op : micro_ops) {
    ++port_masks[PortMask(op.port_mask())];
  }
  return port_masks;
}

// Subtract the micro-ops in rhs (represented by their PortMasks) from those in
// lhs. Returns a bad status if lhs does not contain at least the micro-ops in
// rhs.
Status SubtractMicroOpsFrom(PortMaskCount rhs,
                            DecompositionSolver::MicroOps* const lhs) {
  RemoveIf(lhs, [&rhs](const MicroOperationProto* op) {
    return --rhs[PortMask(op->port_mask())] >= 0;
  });
  for (const auto& remaining_count : rhs) {
    if (remaining_count.second > 0) {
      return InternalError(
          absl::StrCat("The measured code does not include the update code ",
                       remaining_count.first.ToString()));
    }
  }
  return OkStatus();
}

Status ComputeItinerariesHelper::ComputeOneItinerary(
    const InstructionProto& instruction,
    const PortMaskCount& update_code_micro_ops, ItineraryProto* const itinerary,
    Stats* const stats) const {
  // The following registers are excluded because they can't be accessed in user
  // mode.
  static const absl::flat_hash_set<std::string>* const kExcludedMovOperands =
      new absl::flat_hash_set<std::string>({"CR0-CR7", "DR0-DR7"});

  const InstructionFormat& vendor_syntax = GetAnyVendorSyntaxOrDie(instruction);
  LOG(INFO) << "Processing " << PrettyPrintInstruction(instruction);
  const std::string& mnemonic = vendor_syntax.mnemonic();
  if (!instruction.feature_name().empty() &&
      !cpu_info_.SupportsFeature(instruction.feature_name())) {
    LOG(INFO) << "Ignoring instruction " << instruction.llvm_mnemonic()
              << " with unsupported feature " << instruction.feature_name();
    return OkStatus();
  }
  // TODO(courbet): read this from cpuinfo.
  if (!instruction.available_in_64_bit()) {
    LOG(INFO) << "Ignoring instruction " << instruction.llvm_mnemonic()
              << " (!available_in_64_bit)";
    return OkStatus();
  }
  if (microarchitecture_.IsProtectedMode(instruction.protection_mode())) {
    LOG(INFO) << "Ignoring instruction " << instruction.llvm_mnemonic()
              << " requiring lower protection mode";
    return OkStatus();
  }
  if (kExcludedInstructions->contains(mnemonic)) {
    LOG(INFO) << "Ignoring blacklisted instruction "
              << ConvertToCodeString(vendor_syntax);
    return OkStatus();
  }
  if (mnemonic == "MOV" &&
      (kExcludedMovOperands->contains(vendor_syntax.operands(0).name()) ||
       kExcludedMovOperands->contains(vendor_syntax.operands(1).name()) ||
       vendor_syntax.operands(0).name() == "Sreg")) {
    LOG(INFO) << "Ignoring instruction with unsupported operands "
              << ConvertToCodeString(vendor_syntax);
    return OkStatus();
  }
  const InstructionFormat asm_syntax = x86::InstantiateOperands(instruction);
  const std::string measured_code = ConvertToCodeString(asm_syntax);
  VLOG(1) << measured_code;
  VLOG(1) << instruction.DebugString();

  // Check that the code assembles correctly before proceeding.
  {
    JitCompiler jit(host_mcpu_);
    const auto fragment = jit.CompileInlineAssemblyFragment(
        measured_code, llvm::InlineAsm::AD_Intel);
    if (!fragment.ok()) {
      stats->IncrementAssemblyErrors();
      return fragment.status();
    }
  }

  const bool touches_memory = TouchesMemory(vendor_syntax);
  if (touches_memory) {
    LOG(INFO)
        << "The measured instruction touches memory, using the update code.";
  }

  PerfResult result;
  CHECK_OK(EvaluateAssemblyString(
      llvm::InlineAsm::AD_Intel, host_mcpu_, parameters_.inner_iterations,
      init_code_, prefix_code_, measured_code,
      touches_memory ? update_code_ : "", /*suffix_code=*/"", cleanup_code_,
      constraints_, &result));

  LOG(INFO) << result.ToString();
  *itinerary->mutable_throughput_observation() =
      CreateObservationVector(result);

  // Some instructions stall the decode pipeline, resulting in invalid port
  // distribution (see b/34701967 and go/cpu-mysteries/alu_16bits).
  if (result.HasTiming("ild_stall.lcp") &&
      result.GetScaledOrDie("ild_stall.lcp") > 0.1) {
    stats->IncrementDecodeStallsErrors();
    return InternalError(
        absl::StrCat("Instruction stalls decode pipeline: ", measured_code));
  }

  DecompositionSolver solver(microarchitecture_);
  if (solver.Run(itinerary->throughput_observation()).ok()) {
    stats->IncrementSolvedProblems(solver);
    LOG(INFO) << "Mixed-Integer Problem solved in " << solver.wall_time()
              << " ms. Optimal objective value = " << solver.objective_value()
              << "\n"
              << solver.DebugString();
    *itinerary->mutable_micro_ops() = solver.GetMicroOps();
    if (touches_memory) {
      const Status status = SubtractMicroOpsFrom(
          update_code_micro_ops, itinerary->mutable_micro_ops());
      if (!status.ok()) {
        stats->IncrementSubtractUpdateCodeErrors();
        return status;
      }
      LOG(INFO) << "After subtracting update code:"
                << PrettyPrintItinerary(*itinerary,
                                        PrettyPrintOptions()
                                            .WithItinerariesOnOneLine(true)
                                            .WithMicroOpLatencies(false)
                                            .WithMicroOpDependencies(false));
    }
    return OkStatus();
  } else {
    stats->IncrementUnsolvedProblems();
    return InternalError(absl::StrCat("Could not decompose instruction ",
                                      measured_code,
                                      " into micro-operations."));
  }
}

Status ComputeItinerariesHelper::ComputeItineraries(
    const InstructionSetProto& instruction_set,
    InstructionSetItinerariesProto* const itineraries) const {
  const StatusOr<PortMaskCount> update_code_micro_ops =
      ComputeUpdateCodeMicroOps();
  if (!update_code_micro_ops.ok()) {
    return InternalError("Failed to compute update code micro-ops");
  }

  Stats stats;
  Status global_status;
  for (int i = 0; i < instruction_set.instructions_size(); ++i) {
    const Status status = ComputeOneItinerary(
        instruction_set.instructions(i), update_code_micro_ops.ValueOrDie(),
        itineraries->mutable_itineraries(i), &stats);
    if (!status.ok()) {
      LOG(ERROR) << status;
      global_status = status;
    }
  }

  LOG(INFO) << stats.DebugString();
  return global_status;
}

}  // namespace

Status ComputeItineraries(const InstructionSetProto& instruction_set,
                          InstructionSetItinerariesProto* const itineraries) {
  CHECK(itineraries != nullptr);
  CHECK_EQ(instruction_set.instructions_size(),
           itineraries->itineraries_size());
  const CpuInfo& host_cpu_info = HostCpuInfoOrDie();
  LOG(INFO) << "Host CPU info: " << host_cpu_info.DebugString();
  const std::string& host_cpu_model_id = host_cpu_info.cpu_model_id();
  const std::string& host_microarchitecture_id =
      GetMicroArchitectureIdForCpuModelOrDie(host_cpu_model_id);

  // Check that we know the details (port masks, ...) of the CPU model.
  const MicroArchitecture* const microarchitecture =
      MicroArchitecture::FromId(host_microarchitecture_id);
  if (microarchitecture == nullptr) {
    return InternalError(absl::StrCat("Nothing known about host CPU model id '",
                                      host_cpu_model_id,
                                      "', cannot compute itineraries."));
  }

  // We can only guarantee that the computed itineraries are going to be valid
  // for the host microarchitecture.
  if (microarchitecture->proto().id() != itineraries->microarchitecture_id()) {
    return InvalidArgumentError(
        absl::StrCat("Host CPU model id '", host_cpu_model_id,
                     "' is not the requested microarchitecture ('",
                     microarchitecture->proto().id(), "' vs '",
                     itineraries->microarchitecture_id(), "'"));
  }

  ComputeItinerariesHelper helper(host_cpu_info, *microarchitecture,
                                  ComputeItinerariesHelper::Parameters());
  return helper.ComputeItineraries(instruction_set, itineraries);
}

}  // namespace itineraries
}  // namespace exegesis
