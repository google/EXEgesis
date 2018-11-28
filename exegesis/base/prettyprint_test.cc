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

#include "exegesis/base/prettyprint.h"

#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "exegesis/util/proto_util.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(PrettyPrintTest, CpuModel) {
  const MicroArchitecture microarchitecture(
      ParseProtoFromStringOrDie<MicroArchitectureProto>(R"proto(
        id: 'hsw'
        port_masks { port_numbers: [ 0, 1, 5, 6 ] }
        port_masks { port_numbers: [ 2, 3, 7 ] }
        protected_mode { user_modes: 3 }
        model_ids: 'intel:06_3F'
      )proto"));
  EXPECT_EQ(PrettyPrintMicroArchitecture(microarchitecture), "hsw");
  EXPECT_EQ(PrettyPrintMicroArchitecture(
                microarchitecture, PrettyPrintOptions().WithCpuDetails(true)),
            "hsw\n"
            "port masks:\n"
            "  P0156\n"
            "  P237");
}

TEST(PrettyPrintSyntaxTest, Avx512) {
  const InstructionFormat proto =
      ParseProtoFromStringOrDie<InstructionFormat>(R"proto(
        mnemonic: "V4FMADDPS"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
          usage: USAGE_READ_WRITE
          register_class: VECTOR_REGISTER_512_BIT
        }
        operands {
          addressing_mode: BLOCK_DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 2048
          name: "zmm2+3"
          usage: USAGE_READ
          register_class: REGISTER_BLOCK_512_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        })proto");
  EXPECT_EQ(PrettyPrintSyntax(proto), "V4FMADDPS zmm1 {k1} {z}, zmm2+3, m128");
}

TEST(PrettyPrintTest, Instruction) {
  const InstructionProto proto =
      ParseProtoFromStringOrDie<InstructionProto>(R"proto(
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
          operands { name: "ymm1" }
          operands { name: "xmmword ptr [rsi]" }
        }
        att_syntax {
          mnemonic: "vbroadcastf128"
          operands { name: '(%rsi)' }
          operands { name: "%ymm1" }
        }
        feature_name: "AVX"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "RM"
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
        })proto");
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

TEST(PrettyPrintTest, MultipleSyntaxes) {
  const InstructionProto proto =
      ParseProtoFromStringOrDie<InstructionProto>(R"proto(
        vendor_syntax {
          mnemonic: "JZ"
          operands {
            addressing_mode: NO_ADDRESSING
            encoding: IMMEDIATE_VALUE_ENCODING
            value_size_bits: 32
            name: "rel32"
            usage: USAGE_READ
          }
        }
        vendor_syntax {
          mnemonic: "JE"
          operands {
            addressing_mode: NO_ADDRESSING
            encoding: IMMEDIATE_VALUE_ENCODING
            value_size_bits: 32
            name: "rel32"
            usage: USAGE_READ
          }
        }
        syntax {
          mnemonic: "je"
          operands { name: "0x10000" }
        }
        att_syntax {
          mnemonic: "je"
          operands { name: "0x10000" }
        }
        available_in_64_bit: true
        legacy_instruction: true
        raw_encoding_specification: "0F 84 cd"
        x86_encoding_specification {
          opcode: 3972
          legacy_prefixes {
            rex_w_prefix: PREFIX_IS_IGNORED
            operand_size_override_prefix: PREFIX_IS_IGNORED
          }
          code_offset_bytes: 4
        })proto");
  EXPECT_EQ(PrettyPrintSyntaxes(proto.vendor_syntax()), "JZ rel32\nJE rel32");
  EXPECT_EQ(PrettyPrintInstruction(
                proto, PrettyPrintOptions().WithAlternativeSyntax(true)),
            "JZ rel32\n"
            "JE rel32\n"
            "intel: je 0x10000\n"
            "att: je 0x10000");
  EXPECT_EQ(PrettyPrintInstruction(
                proto, PrettyPrintOptions().WithVendorSyntaxesOnOneLine(true)),
            "JZ rel32; JE rel32");
}

TEST(PrettyPrintTest, Itinerary) {
  const ItineraryProto proto =
      ParseProtoFromStringOrDie<ItineraryProto>(R"proto(
        micro_ops {
          port_mask { port_numbers: [ 0, 1, 5, 6 ] }
          latency: 2
        }
        micro_ops { port_mask { port_numbers: [ 2, 3, 7 ] } }
        micro_ops {
          port_mask { port_numbers: [ 4 ] }
          dependencies: [ 0, 1 ]
        }
      )proto");
  EXPECT_EQ(PrettyPrintItinerary(proto, PrettyPrintOptions()),
            "  P0156 (lat:2)\n"
            "  P237 (lat:0)\n"
            "  P4 (lat:0) (deps:0,1)");
}

TEST(PrettyPrintTest, ItineraryOneLine) {
  const ItineraryProto proto =
      ParseProtoFromStringOrDie<ItineraryProto>(R"proto(
        micro_ops {
          port_mask { port_numbers: [ 0, 1, 5, 6 ] }
          latency: 2
        }
        micro_ops { port_mask { port_numbers: [ 2, 3, 7 ] } }
        micro_ops {
          port_mask { port_numbers: [ 4 ] }
          dependencies: [ 0, 1 ]
        }
      )proto");
  EXPECT_EQ(PrettyPrintItinerary(proto, PrettyPrintOptions()
                                            .WithItinerariesOnOneLine(true)
                                            .WithMicroOpLatencies(false)
                                            .WithMicroOpDependencies(false)),
            "P0156 P237 P4");
}

}  // namespace
}  // namespace exegesis
