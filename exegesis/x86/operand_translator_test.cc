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

#include "exegesis/x86/operand_translator.h"

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;

TEST(OperandTranslatorTest, Works) {
  const auto instruction = ParseProtoFromStringOrDie<InstructionProto>(R"proto(
    legacy_instruction: true
    vendor_syntax {
      mnemonic: 'ADD'
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: MODRM_REG_ENCODING
        value_size_bits: 32
        name: 'r32'
      }
      operands {
        addressing_mode: NO_ADDRESSING
        encoding: IMMEDIATE_VALUE_ENCODING
        value_size_bits: 8
        name: 'imm8'
      }
    })proto");
  constexpr const char kExpected[] =
      R"proto(
    mnemonic: 'ADD'
    operands { name: 'ecx' }
    operands { name: '0x11' })proto";
  EXPECT_THAT(InstantiateOperands(instruction), EqualsProto(kExpected));
}

TEST(OperandTranslatorTest, Avx512) {
  const auto instruction = ParseProtoFromStringOrDie<InstructionProto>(R"proto(
    vendor_syntax {
      mnemonic: "VPADDB"
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: MODRM_REG_ENCODING
        value_size_bits: 128
        name: "xmm1"
        tags { name: "k1" }
        tags { name: "z" }
        usage: USAGE_WRITE
      }
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: VEX_V_ENCODING
        value_size_bits: 128
        name: "xmm2"
        usage: USAGE_READ
      }
      operands {
        addressing_mode: INDIRECT_ADDRESSING
        encoding: MODRM_RM_ENCODING
        value_size_bits: 128
        name: "m128"
        usage: USAGE_READ
      }
    }
    available_in_64_bit: true
    legacy_instruction: true
    raw_encoding_specification: "EVEX.NDS.128.66.0F.WIG FC /r")proto");
  constexpr char kExpectedFormat[] = R"proto(
    mnemonic: "VPADDB"
    operands {
      name: "xmm1"
      tags { name: "k1" }
      tags { name: "z" }
    }
    operands { name: "xmm2" }
    operands { name: "xmmword ptr[RSI]" }
  )proto";
  EXPECT_THAT(InstantiateOperands(instruction), EqualsProto(kExpectedFormat));
}

TEST(OperandTranslatorTest, Avx512StaticRounding) {
  const auto instruction = ParseProtoFromStringOrDie<InstructionProto>(R"proto(
    vendor_syntax {
      mnemonic: "VADDPD"
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: MODRM_REG_ENCODING
        value_size_bits: 512
        name: "zmm1"
        tags { name: "k1" }
        tags { name: "z" }
        usage: USAGE_WRITE
      }
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: VEX_V_ENCODING
        value_size_bits: 512
        name: "zmm2"
        usage: USAGE_READ
      }
      operands {
        addressing_mode: DIRECT_ADDRESSING
        encoding: MODRM_RM_ENCODING
        value_size_bits: 512
        name: "zmm3"
        usage: USAGE_READ
      }
      operands {
        addressing_mode: NO_ADDRESSING
        encoding: X86_STATIC_PROPERTY_ENCODING
        usage: USAGE_READ
        tags { name: "er" }
      }
    }
    available_in_64_bit: true
    raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 58 /r")proto");
  constexpr char kExpectedFormat[] = R"proto(
    mnemonic: "VADDPD"
    operands {
      name: "zmm1"
      tags { name: "k1" }
      tags { name: "z" }
    }
    operands { name: "zmm2" }
    operands { name: "zmm3" }
    operands { tags { name: "rn-sae" } })proto";
  EXPECT_THAT(InstantiateOperands(instruction), EqualsProto(kExpectedFormat));
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
