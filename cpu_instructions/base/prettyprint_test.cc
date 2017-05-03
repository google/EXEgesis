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

#include "cpu_instructions/base/prettyprint.h"

#include "cpu_instructions/proto/instructions.pb.h"
#include "cpu_instructions/proto/microarchitecture.pb.h"
#include "cpu_instructions/util/proto_util.h"
#include "gtest/gtest.h"

namespace cpu_instructions {
namespace {

TEST(PrettyPrintTest, CpuModel) {
  const MicroArchitecture microarchitecture(
      ParseProtoFromStringOrDie<MicroArchitectureProto>(R"(
      id: 'haswell'
      port_masks { port_numbers: [0, 1, 5, 6] }
      port_masks { port_numbers: [2, 3, 7] }
      protected_mode { user_modes: 3 }
      cpu_models {
        id: 'intel:06_3F'
        code_name: 'haswell'
      }
  )"));
  EXPECT_EQ(PrettyPrintCpuModel(microarchitecture.cpu_models()[0]),
            "intel:06_3F (name: 'haswell')");
  EXPECT_EQ(PrettyPrintCpuModel(microarchitecture.cpu_models()[0],
                                PrettyPrintOptions().WithCpuDetails(true)),
            "intel:06_3F (name: 'haswell')\n"
            "port masks:\n"
            "  P0156\n"
            "  P237");
}

TEST(PrettyPrintTest, Instruction) {
  const InstructionProto proto = ParseProtoFromStringOrDie<InstructionProto>(R"(
      description: "Blah"
      llvm_mnemonic: "VBROADCASTF128"
      vendor_syntax {
        mnemonic: "VBROADCASTF128"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 256
          name: "ymm1"
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
      }
      syntax {
        mnemonic: "vbroadcastf128"
        operands {
          name: "ymm1"
        }
        operands {
          name: "xmmword ptr [rsi]"
        }
      }
      att_syntax {
        mnemonic: "vbroadcastf128"
        operands {
          name: '(%rsi)'
        }
        operands {
          name: "%ymm1"
        }
      }
      feature_name: "AVX"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      binary_encoding_size_bytes: 5
      raw_encoding_specification: "VEX.256.66.0F38.W0 1A /r"
      x86_encoding_specification {
        opcode: 997402
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_256_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
      group_id: "VBROADCAST-Load with Broadcast Floating-Point Data"
  )");
  EXPECT_EQ(PrettyPrintInstruction(proto, PrettyPrintOptions()),
            "VBROADCASTF128 ymm1, m128\n"
            "llvm: VBROADCASTF128");
  EXPECT_EQ(PrettyPrintInstruction(
                proto, PrettyPrintOptions().WithAlternativeSyntax(true)),
            "VBROADCASTF128 ymm1, m128\n"
            "llvm: VBROADCASTF128\n"
            "intel: vbroadcastf128 ymm1, xmmword ptr [rsi]\n"
            "att: vbroadcastf128 (%rsi), %ymm1");
}

TEST(PrettyPrintTest, Itinerary) {
  const ItineraryProto proto = ParseProtoFromStringOrDie<ItineraryProto>(R"(
      micro_ops {
        port_mask { port_numbers: [0, 1, 5, 6]}
        latency: 2
      }
      micro_ops {
        port_mask { port_numbers: [2,3,7]}
      }
      micro_ops {
        port_mask { port_numbers: [4]}
        dependencies: [0, 1]
      }
  )");
  EXPECT_EQ(PrettyPrintItinerary(proto, PrettyPrintOptions()),
            "  P0156 (lat:2)\n"
            "  P237\n"
            "  P4 (deps:0,1)");
}

TEST(PrettyPrintTest, ItineraryOneLine) {
  const ItineraryProto proto = ParseProtoFromStringOrDie<ItineraryProto>(R"(
      micro_ops {
        port_mask { port_numbers: [0, 1, 5, 6]}
        latency: 2
      }
      micro_ops {
        port_mask { port_numbers: [2,3,7]}
      }
      micro_ops {
        port_mask { port_numbers: [4]}
        dependencies: [0, 1]
      }
  )");
  EXPECT_EQ(PrettyPrintItinerary(proto, PrettyPrintOptions()
                                            .WithItinerariesOnOneLine(true)
                                            .WithMicroOpLatencies(false)
                                            .WithMicroOpDependencies(false)),
            "P0156 P237 P4");
}

}  // namespace
}  // namespace cpu_instructions
