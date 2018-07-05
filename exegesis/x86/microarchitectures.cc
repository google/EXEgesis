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
#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "glog/logging.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace x86 {
namespace {

// This is derived from Figure 2-1 "CPU Core Pipeline Functionality of the
// Skylake Microarchitecture" and Table 2-1. "Dispatch Port and Execution Stacks
// of the Skylake Microarchitecture" of the June 2016 edition of the Intel
// Optimization Reference Manual, Order Number 248966-033.
// http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-optimization-manual.pdf
constexpr const char kSkylakeMicroArchitecture[] = R"proto(
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
)proto";

constexpr const char kSkylakeConsumerModels[] =
    R"proto(
  id: "skl" model_ids: 'intel:06_4E' model_ids: 'intel:06_5E'
    )proto";

constexpr const char kSkylakeXeonModels[] = R"proto(
  id: "skx"
  model_ids: 'intel:06_55'
)proto";

// The Haswell CPU microarchitecture.
constexpr const char kHaswellMicroArchitecture[] = R"proto(
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
)proto";

constexpr const char kHaswellModels[] = R"proto(
  id: "hsw"
  model_ids: 'intel:06_3C'
  model_ids: 'intel:06_3F'
  model_ids: 'intel:06_45'
  model_ids: 'intel:06_46'
)proto";

constexpr const char kBroadwellModels[] = R"proto(
  id: "bdw"
  model_ids: 'intel:06_3D'
  model_ids: 'intel:06_47'
  model_ids: 'intel:06_56'
)proto";

constexpr const char kSandyBridgeMicroArchitecture[] = R"proto(
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
)proto";

constexpr const char kIvyBridgeModels[] =
    R"proto(
  id: "ivb" model_ids: 'intel:06_3A' model_ids: 'intel:06_3E'
    )proto";

constexpr const char kSandyBridgeModels[] =
    R"proto(
  id: "snb" model_ids: 'intel:06_2A' model_ids: 'intel:06_2D'
    )proto";

constexpr const char kNehalemMicroArchitecture[] = R"proto(
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
)proto";

constexpr const char kWestmireModels[] = R"proto(
  id: "wsm"
  model_ids: 'intel:06_25'
  model_ids: 'intel:06_2C'
  model_ids: 'intel:06_2F'
)proto";

constexpr const char kNehalemModels[] = R"proto(
  id: "nhm"
  model_ids: 'intel:06_1A'
  model_ids: 'intel:06_1E'
  model_ids: 'intel:06_1F'
  model_ids: 'intel:06_2E'
)proto";

constexpr const char kEnhancedCoreModels[] = R"proto(
  id: "enhanced_core"
  model_ids: 'intel:06_17'
  model_ids: 'intel:06_1D'
)proto";

constexpr const char kCoreModels[] = R"proto(
  id: "core" model_ids: 'intel:06_0F'
)proto";

const MicroArchitecturesProto& GetMicroArchitecturesProto() {
  static const MicroArchitecturesProto* const microarchitectures = []() {
    const std::vector<std::string> sources = {
        absl::StrCat(kSkylakeConsumerModels, kSkylakeMicroArchitecture),
        absl::StrCat(kSkylakeXeonModels, kSkylakeMicroArchitecture),
        absl::StrCat(kHaswellModels, kHaswellMicroArchitecture),
        absl::StrCat(kBroadwellModels, kHaswellMicroArchitecture),
        absl::StrCat(kIvyBridgeModels, kSandyBridgeMicroArchitecture),
        absl::StrCat(kSandyBridgeModels, kSandyBridgeMicroArchitecture),
        absl::StrCat(kWestmireModels, kNehalemMicroArchitecture),
        absl::StrCat(kNehalemModels, kNehalemMicroArchitecture),
        // NOTE(bdb): As of 2017-03-01 we do not need the itineraries of the
        // Core and Enhanced Core architectures.
        kEnhancedCoreModels, kCoreModels};
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
