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

#include "exegesis/x86/microarchitectures.h"

#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "glog/logging.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace x86 {
namespace {

// Port definitions and port masks are based on the description of the Ice Lake
// microarchitecture in the Intel(r) 64 and IA-32 Architectures Optimization
// Reference Manual, May 2020 version, Table 2-1 and Table 2-2.
constexpr absl::string_view kIceLakeMicroArchitecture = R"pb(
  # Port 0
  ports {
    comments: "Integer ALU"
    comments: "Fast LEA"
    comments: "Integer Shift"
    comments: "Branch"
    comments: "FMA"
    comments: "Vector ALU"
    comments: "Vector Shifts"
    comments: "FP Divide"
  }
  # Port 1
  ports {
    comments: "Integer ALU"
    comments: "Fast LEA"
    comments: "Integer Multiply"
    comments: "Integer Divison"
    comments: "FMA (no AVX-512)"
    comments: "Vector ALU (no AVX-512)"
    comments: "Vector Shifts (no AVX-512)"
    comments: "Vector Shuffle (no AVX-512)"
  }
  # Port 2
  ports { comments: "Load" }
  # Port 3
  ports { comments: "Load" }
  # Port 4
  ports { comments: "Store Data" }
  # Port 5
  ports {
    comments: "Fast LEA"
    comments: "Integer ALU"
    comments: "Integer Multiply Hi"
    comments: "Vector ALU"
    comments: "Vector Shuffle"
  }
  # Port 6
  ports {
    comments: "Integer ALU"
    comments: "Fast LEA"
    comments: "Integer Shift"
    comments: "Branch"
  }
  # Port 7
  ports { comments: "Store Address" }
  # Port 8
  ports { comments: "Store Address" }
  # Port 9
  ports { comments: "Store Data" }

  # TODO(ondrasej): Verify the port masks using llvm-exegesis.
  port_masks {
    # Integer ALU: add, and, cmp, or, test, xor, movzx, movsx, mov, (v)movdqu,
    # (v)movdqa, (v)movap*, (v)movup*
    comment: "ALU"
    port_numbers: [ 0, 1, 5, 6 ]
  }
  port_masks {
    # Integer Shift: sal, shl, rol, adc, sarx, adcx, adox, etc.
    comment: "Integer Shift"
    port_numbers: [ 0, 6 ]
  }
  port_masks {
    # Integer Multiply and other slow instructions: mul, imul, bsr, rcl, shld,
    # mulx, pdep, etc.
    port_numbers: 1
  }
  port_masks {
    # Vector ALU:  (v)pand, (v)por, (v)pxor, (v)movq, (v)movq, (v)movap*,
    # (v)movup*, (v)andp*, (v)orp*, (v)paddb/w/d/q, (v)blendv*, (v)blendp*,
    # (v)pblendd
    port_numbers: [ 0, 1, 5 ]
  }
  port_masks {
    # Vector Shift: (v)psllv*, (v)psrlv*
    port_numbers: [ 0, 1 ]
  }
  port_masks {
    # Vector Shuffle: (v)shufp*, vperm*, (v)pack*, (v)unpck*, (v)punpck*,
    # (v)pshuf*, (v)pslldq, (v)alignr, (v)pmovzx*, vbroadcast*, (v)pslldq,
    # (v)psrldq, (v)pblendw
    port_numbers: [ 1, 5 ]
  }
  port_masks {
    # AVX-512 instructions, division.
    port_numbers: 0
  }
  port_masks {
    # Load + load address generation.
    port_numbers: [ 2, 3 ]
  }
  port_masks {
    # Store data.
    port_numbers: [ 4, 9 ]
  }
  port_masks {
    # Store address generation.
    port_numbers: [ 7, 8 ]
  }
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  load_store_address_generation_port_mask_index: 7
  store_address_generation_port_mask_index: 9
  store_data_port_mask_index: 8
  perf_events {
    # TODO(bdb): Only consider user-time measurements with the :u modifier.
    computation_events: "uops_dispatched_port:port_0"
    computation_events: "uops_dispatched_port:port_1"
    computation_events: "uops_dispatched_port:port_5"
    computation_events: "uops_dispatched_port:port_6"
    memory_events: "uops_dispatched_port:port_2_3"
    memory_events: "uops_dispatched_port:port_4_9"
    memory_events: "uops_dispatched_port:port_7_8"
    cycle_events: "cycles"
    cycle_events: "instructions"
    cycle_events: "ild_stall.lcp"
    uops_events: "uops_issued:slots"
    uops_events: "uops_retired:all"
  })pb";

constexpr absl::string_view kIceLakeConsumerModels = R"pb(
  id: "clk"
  llvm_arch: "x86_64"
  llvm_cpu: "icelake-client"
  model_ids: "intel:06_7D"
  model_ids: "intel:06_7E")pb";

constexpr absl::string_view kIceLakeXeonModels = R"pb(
  id: "clx"
  llvm_arch: "x86_64"
  llvm_cpu: "icelake-server"
  model_ids: "intel:06_6A"
  model_ids: "intel:06_6C"
)pb";

// This is derived from Figure 2-1 "CPU Core Pipeline Functionality of the
// Skylake Microarchitecture" and Table 2-1. "Dispatch Port and Execution Stacks
// of the Skylake Microarchitecture" of the June 2016 edition of the Intel
// Optimization Reference Manual, Order Number 248966-033.
// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-optimization-manual.pdf
constexpr absl::string_view kSkylakeMicroArchitecture = R"pb(
  ports {
    comments: "Integer ALU"
    comments: "Integer Shift"
    comments: "Branch"
    comments: "Vector FMA"
    comments: "Vector Multiply"
    comments: "Vector Add"
    comments: "Vector ALU"
    comments: "Vector Shifts"
    comments: "Vector Divide"
  }
  ports {
    comments: "Integer ALU"
    comments: "Fast LEA"
    comments: "Integer Multiply"
    comments: "Vector FMA"
    comments: "Vector Multiply"
    comments: "Vector Add"
    comments: "Vector ALU"
    comments: "Vector Shifts"
    comments: "Slow LEA"
  }
  ports { comments: "Load & Store Address" }
  ports { comments: "Load & Store Address" }
  ports { comments: "Store Data" }
  ports {
    comments: "Integer ALU"
    comments: "Fast LEA"
    comments: "Vector Shuffle"
    comments: "Vector ALU"
    comments: "CVT"
  }
  ports { comments: "Integer ALU" comments: "Integer Shift" comments: "Branch" }
  ports { comments: "Store Address" }
  port_masks {
    # Divide: divp*, divs*, vdiv*, sqrt*, vsqrt*, rcp*, vrcp*, rsqrt*, idiv
    comment: "Divide, vector int multiply, vector shifts."
    port_numbers: 0
  }
  port_masks {
    # (v)mul*, (v)pmul*, (v)pmadd*,
    # (v)movsd/ss, (v)movd gpr,
    comment: "FMA, FP multiply, FP load, Vector Multiply"
    port_numbers: [ 0, 1 ]
  }
  port_masks {
    # (v)pand, (v)por, (v)pxor, (v)movq, (v)movq, (v)movap*, (v)movup*,
    # (v)andp*, (v)orp*, (v)paddb/w/d/q, (v)blendv*, (v)blendp*, (v)pblendd
    comment: "Vector ALU."
    port_numbers: [ 0, 1, 5 ]
  }
  port_masks {
    # add, and, cmp, or, test, xor, movzx, movsx, mov, (v)movdqu, (v)movdqa,
    # (v)movap*, (v)movup*
    comment: "Integer ALU."
    port_numbers: [ 0, 1, 5, 6 ]
  }
  port_masks {
    # Shifts: sal, shl, rol, adc, sarx, adcx, adox, etc.
    comment: "Jcc & fused arithmetic (predicted not taken). Integer shift."
    port_numbers: [ 0, 6 ]
  }
  port_masks {
    # mul, imul, bsr, rcl, shld, mulx, pdep, etc.
    comment: "Slow int, FP add. LEA (RIP or 3 components in address)."
    port_numbers: 1
  }
  port_masks {
    # (v)addp*, (v)cmpp*, (v)max*, (v)min*, (v)padds*, (v)paddus*, (v)psign,
    # (v)pabs, (v)pavgb, (v)pcmpeq*, (v)pmax, (v)cvtps2dq, (v)cvtdq2ps,
    # (v)cvtsd2si, (v)cvtss2s
    comment: "Vector int ALU. Integer LEA (2 components in address)."
    port_numbers: [ 1, 5 ]
  }
  port_masks {
    comment: "Load/store address generation."
    port_numbers: [ 2, 3 ]
  }
  port_masks {
    comment: "Store address generation."
    port_numbers: [ 2, 3, 7 ]
  }
  port_masks { comment: "Store data." port_numbers: 4 }
  port_masks {
    # (v)shufp*, vperm*, (v)pack*, (v)unpck*, (v)punpck*, (v)pshuf*,
    # (v)pslldq, (v)alignr, (v)pmovzx*, vbroadcast*, (v)pslldq, (v)psrldq,
    # (v)pblendw
    comment: "Vector shuffle."
    port_numbers: 5
  }
  port_masks {
    comment: "Partial integer ALU (AAM, MUL, DIV). "
             "JMP, Jcc & fused arithmetic predicted taken."
    port_numbers: 6
  }
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  load_store_address_generation_port_mask_index: 8
  store_address_generation_port_mask_index: 9
  store_data_port_mask_index: 10
  perf_events {
    # TODO(bdb): Only consider user-time measurements with the :u modifier.
    computation_events: "uops_dispatched_port:port_0"
    computation_events: "uops_dispatched_port:port_1"
    computation_events: "uops_dispatched_port:port_5"
    computation_events: "uops_dispatched_port:port_6"
    memory_events: "uops_dispatched_port:port_2"
    memory_events: "uops_dispatched_port:port_3"
    memory_events: "uops_dispatched_port:port_4"
    memory_events: "uops_dispatched_port:port_7"
    cycle_events: "cycles"
    cycle_events: "instructions"
    cycle_events: "ild_stall.lcp"
    uops_events: "uops_issued:any"
    uops_events: "uops_retired:all"
  }
)pb";

constexpr absl::string_view kSkylakeConsumerModels =
    R"pb(
  id: "skl"
  llvm_arch: "x86_64"
  llvm_cpu: "skylake"
  model_ids: 'intel:06_4E'
  model_ids: 'intel:06_5E'
    )pb";

constexpr absl::string_view kSkylakeXeonModels =
    R"pb(
  id: "skx"
  llvm_arch: "x86_64"
  llvm_cpu: "skylake-avx512"
  model_ids: 'intel:06_55'
    )pb";

// The Haswell CPU microarchitecture.
constexpr absl::string_view kHaswellMicroArchitecture = R"pb(
  ports {
    comments: "Integer ALU & Shift"
    comments: "FMA, 256-bit FP Multiply"
    comments: "Vector Int Multiply"
    comments: "Vector Logicals"
    comments: "Branch"
    comments: "Divide"
    comments: "Vector Shifts"
  }
  ports {
    comments: "Integer ALU & LEA"
    comments: "FMA, FP Multiply, 256-bit FP Add"
    comments: "Vector Int ALU"
    comments: "Vector Logicals"
  }
  ports { comments: "Load & Store Address" }
  ports { comments: "Load & Store Address" }
  ports { comments: "Store Data" }
  ports {
    comments: "Integer ALU & LEA"
    comments: "Vector Shuffle"
    comments: "Vector Int ALU"
    comments: "256-bit Vector Logicals"
  }
  ports { comments: "Integer ALU & Shift" comments: "Branch" }
  ports { comments: "Store Address" }
  port_masks {
    comment: "Divide, vector shifts, vector int multiply, vector shifts."
    port_numbers: 0
  }
  port_masks {
    comment: "FMA, FP multiply, FP load."
    port_numbers: [ 0, 1 ]
  }
  port_masks {
    comment: "Vector logicals."
    port_numbers: [ 0, 1, 5 ]
  }
  port_masks {
    comment: "Integer ALU."
    port_numbers: [ 0, 1, 5, 6 ]
  }
  port_masks {
    comment: "Jcc & fused arithmetic (predicted not taken). Integer shift."
    port_numbers: [ 0, 6 ]
  }
  port_masks {
    comment: "FP add. LEA (RIP or 3 components in address)."
    port_numbers: 1
  }
  port_masks {
    comment: "Vector int ALU. Integer LEA (2 components in address)."
    port_numbers: [ 1, 5 ]
  }
  port_masks {
    comment: "Load/store address generation."
    port_numbers: [ 2, 3 ]
  }
  port_masks {
    comment: "Store address generation."
    port_numbers: [ 2, 3, 7 ]
  }
  port_masks { comment: "Store data." port_numbers: 4 }
  port_masks { comment: "Vector shuffle." port_numbers: 5 }
  port_masks {
    comment: "Partial integer ALU (AAM, MUL, DIV). JMP, Jcc & fused arithmetic predicted taken."
    port_numbers: 6
  }
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  load_store_address_generation_port_mask_index: 8
  store_address_generation_port_mask_index: 9
  store_data_port_mask_index: 10
  perf_events {
    # TODO(bdb): Only consider user-time measurements with the :u modifier.
    computation_events: "uops_executed_port:port_0"
    computation_events: "uops_executed_port:port_1"
    computation_events: "uops_executed_port:port_5"
    computation_events: "uops_executed_port:port_6"
    memory_events: "uops_executed_port:port_2"
    memory_events: "uops_executed_port:port_3"
    memory_events: "uops_executed_port:port_4"
    memory_events: "uops_executed_port:port_7"
    cycle_events: "cycles"
    cycle_events: "instructions"
    cycle_events: "ild_stall.lcp"
    uops_events: "uops_issued:any"
    uops_events: "uops_retired:all"
  }

  num_instructions_parsed_per_cycle: 6
  num_bytes_parsed_per_cycle: 16
  parsed_instruction_queue_capacity: 20
  num_simple_instructions_decoded_per_cycle: 3
  num_complex_instructions_decoded_per_cycle: 1
  reorder_buffer_size_in_uops: 192
  reservation_station_size_in_uops: 60
  num_execution_ports: 8
)pb";

constexpr absl::string_view kHaswellModels = R"pb(
  id: "hsw"
  llvm_arch: "x86_64"
  llvm_cpu: "haswell"
  model_ids: 'intel:06_3C'
  model_ids: 'intel:06_3F'
  model_ids: 'intel:06_45'
  model_ids: 'intel:06_46'
)pb";

constexpr absl::string_view kBroadwellModels = R"pb(
  id: "bdw"
  llvm_arch: "x86_64"
  llvm_cpu: "broadwell"
  model_ids: 'intel:06_3D'
  model_ids: 'intel:06_47'
  model_ids: 'intel:06_56'
)pb";

constexpr absl::string_view kSandyBridgeMicroArchitecture = R"pb(
  ports {
    comments: "Integer ALU"
    comments: "Shift"
    comments: "256-bit FP Multiply"
    comments: "Vector Int Multiply"
    comments: "Vector Logicals"
    comments: "Vector Shifts"
    comments: "Divide"
  }
  ports {
    comments: "Integer ALU & LEA"
    comments: "256-bit FP Add"
    comments: "Vector Int ALU"
    comments: "Vector Logicals"
  }
  ports { comments: "Load/Store Address" }
  ports { comments: "Load/Store Address" }
  ports { comments: "Store Data" }
  ports {
    comments: "Integer ALU"
    comments: "Shift"
    comments: "Vector Int ALU"
    comments: "256-bit Vector Logicals"
    comments: "Branch"
  }
  port_masks {
    comment: "Divide, vector shifts, vector int multiply, vector shifts, "
             "FP multiply, Jcc & fused arithmetic, JMP."
    port_numbers: 0
  }
  port_masks {
    comment: "Vector logicals, Integer ALU."
    port_numbers: [ 0, 1, 5 ]
  }
  port_masks {
    comment: "FP add. LEA (RIP or 3 components in address)."
    port_numbers: 1
  }
  port_masks {
    comment: "Vector int ALU. Integer LEA (2 components in address)."
    port_numbers: [ 1, 5 ]
  }
  port_masks {
    comment: "Load/store address generation."
    port_numbers: [ 2, 3 ]
  }
  port_masks { comment: "Store data." port_numbers: 4 }
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  load_store_address_generation_port_mask_index: 5
  store_address_generation_port_mask_index: 5
  store_data_port_mask_index: 6
  perf_events {
    # TODO(bdb): Only consider user-time measurements with the :u modifier.
    computation_events: "uops_dispatched_port:port_0"
    computation_events: "uops_dispatched_port:port_1"
    computation_events: "uops_dispatched_port:port_5"
    memory_events: "uops_dispatched_port:port_2"
    memory_events: "uops_dispatched_port:port_3"
    memory_events: "uops_dispatched_port:port_4"
    cycle_events: "cycles"
    cycle_events: "instructions"
    cycle_events: "ild_stall.lcp"
    uops_events: "uops_issued:any"
    uops_events: "uops_retired:all"
  }
)pb";

constexpr absl::string_view kIvyBridgeModels =
    R"pb(
  id: "ivb"
  llvm_arch: "x86_64"
  llvm_cpu: "ivybridge"
  model_ids: 'intel:06_3A'
  model_ids: 'intel:06_3E'
    )pb";

constexpr absl::string_view kSandyBridgeModels =
    R"pb(
  llvm_arch: "x86_64"
  llvm_cpu: "sandybridge"
  id: "snb"
  model_ids: 'intel:06_2A'
  model_ids: 'intel:06_2D'
    )pb";

constexpr absl::string_view kNehalemMicroArchitecture = R"pb(
  ports {
    comments: "Integer ALU"
    comments: "Shift"
    comments: "FP Multiply"
    comments: "Vector Int Multiply"
    comments: "Vector Logicals"
    comments: "Vector Shifts"
    comments: "Divide"
  }
  ports {
    comments: "Integer ALU & LEA"
    comments: "FP Add"
    comments: "Vector Int ALU"
    comments: "Vector Logicals"
  }
  ports { comments: "Load" }
  ports { comments: "Store Address" }
  ports { comments: "Store Data" }
  ports {
    comments: "Integer ALU"
    comments: "Shift"
    comments: "Vector Int ALU"
    comments: "Vector Logicals"
    comments: "Branch"
  }
  port_masks {
    comment: "Divide, vector shifts, vector int multiply, vector shifts, "
             "FP multiply, Jcc & fused arithmetic, JMP."
    port_numbers: 0
  }
  port_masks {
    comment: "Vector logicals, Integer ALU."
    port_numbers: [ 0, 1, 5 ]
  }
  port_masks {
    comment: "FP add. LEA (RIP or 3 components in address)."
    port_numbers: 1
  }
  port_masks {
    comment: "Vector int ALU. Integer LEA (2 components in address)."
    port_numbers: [ 1, 5 ]
  }
  port_masks { comment: "Load." port_numbers: 2 }
  port_masks { comment: "Store address generation." port_numbers: 3 }
  port_masks { comment: "Store data." port_numbers: 4 }
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  load_store_address_generation_port_mask_index: 5
  store_address_generation_port_mask_index: 6
  store_data_port_mask_index: 7
  perf_events {
    # TODO(bdb): Only consider user-time measurements with the :u modifier.
    computation_events: "uops_executed:port0"
    computation_events: "uops_executed:port1"
    computation_events: "uops_executed:port5"
    computation_events: "uops_executed:port015"  # WTF ?
    memory_events: "uops_executed:port2"
    memory_events: "uops_executed:port3"
    memory_events: "uops_executed:port4"
    cycle_events: "cycles"
    cycle_events: "instructions"
    cycle_events: "ild_stall.lcp"
    uops_events: "uops_issued"
    uops_events: "uops_retired"
  }
)pb";

constexpr absl::string_view kWestmereModels = R"pb(
  id: "wsm"
  llvm_arch: "x86_64"
  llvm_cpu: "westmere"
  model_ids: 'intel:06_25'
  model_ids: 'intel:06_2C'
  model_ids: 'intel:06_2F'
)pb";

constexpr absl::string_view kNehalemModels = R"pb(
  id: "nhm"
  llvm_arch: "x86_64"
  llvm_cpu: "nehalem"
  model_ids: 'intel:06_1A'
  model_ids: 'intel:06_1E'
  model_ids: 'intel:06_1F'
  model_ids: 'intel:06_2E'
)pb";

constexpr absl::string_view kEnhancedCoreModels =
    R"pb(
  id: "enhanced_core" model_ids: 'intel:06_17' model_ids: 'intel:06_1D'
)pb";

constexpr absl::string_view kCoreModels = R"pb(
  id: "core"
  model_ids: 'intel:06_0F'
)pb";

// Perf counter definitions for AMD Zen CPUs. These are based on the definitions
// in llvm-exegesis.
constexpr absl::string_view kAmdZenMicroArchitectureAndModels = R"pb(
  id: "zen"
  llvm_arch: "x86_64"
  llvm_cpu: "znver1"
  model_ids: 'intel:8f_01'
  model_ids: 'intel:8f_11'
  model_ids: 'intel:8f_18'
  model_ids: 'intel:8f_20'
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  # AMD Zen CPUs do not provide detailed execution unit perf counters. We thus
  # skip port definitions and port masks.
)pb";

constexpr absl::string_view kAmdZen2MicroArchitectureAndModels = R"pb(
  id: "zen2"
  llvm_arch: "x86_64"
  llvm_cpu: "znver2"
  model_ids: 'intel:8F_31'
  model_ids: 'intel:8F_60'
  model_ids: 'intel:8F_71'
  protected_mode { protected_modes: [ 0, 1, 2 ] }
  # AMD Zen 2 CPUs do not provide detailed execution unit perf counters. We thus
  # skip port definitions and port masks.
)pb";

const MicroArchitecturesProto& GetMicroArchitecturesProto() {
  static const MicroArchitecturesProto* const microarchitectures = []() {
    const std::vector<std::string> sources = {
        absl::StrCat(kIceLakeConsumerModels, kIceLakeMicroArchitecture),
        absl::StrCat(kIceLakeXeonModels, kIceLakeMicroArchitecture),
        absl::StrCat(kSkylakeConsumerModels, kSkylakeMicroArchitecture),
        absl::StrCat(kSkylakeXeonModels, kSkylakeMicroArchitecture),
        absl::StrCat(kHaswellModels, kHaswellMicroArchitecture),
        absl::StrCat(kBroadwellModels, kHaswellMicroArchitecture),
        absl::StrCat(kIvyBridgeModels, kSandyBridgeMicroArchitecture),
        absl::StrCat(kSandyBridgeModels, kSandyBridgeMicroArchitecture),
        absl::StrCat(kWestmereModels, kNehalemMicroArchitecture),
        absl::StrCat(kNehalemModels, kNehalemMicroArchitecture),
        // NOTE(bdb): As of 2017-03-01 we do not need the itineraries of the
        // Core and Enhanced Core architectures.
        std::string(kEnhancedCoreModels), std::string(kCoreModels),
        std::string(kAmdZenMicroArchitectureAndModels),
        std::string(kAmdZen2MicroArchitectureAndModels)};
    auto* const result = new MicroArchitecturesProto();
    for (const std::string& source : sources) {
      CHECK(::google::protobuf::TextFormat::ParseFromString(
          source, result->add_microarchitectures()));
    }
    return result;
  }();
  return *microarchitectures;
}
REGISTER_MICRO_ARCHITECTURES(GetMicroArchitecturesProto);

}  // namespace
}  // namespace x86
}  // namespace exegesis
