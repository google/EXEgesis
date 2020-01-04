// Copyright 2016 Google Inc.
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

// Contains positive and negative tests for the EncodingSpecification
// class and GetAvailableEncodings.

#include "exegesis/x86/encoding_specification.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_set>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::IsOk;
using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::util::StatusOr;
using ::testing::Not;
using ::testing::ResultOf;
using ::testing::UnorderedElementsAreArray;

void CheckParser(const std::string& specification_str,
                 const std::string& expected_specification_proto) {
  SCOPED_TRACE(absl::StrCat("Specification: ", specification_str));
  EXPECT_THAT(ParseEncodingSpecification(specification_str),
              IsOkAndHolds(EqualsProto(expected_specification_proto)));
}

void CheckParserFailure(const std::string& specification_str) {
  SCOPED_TRACE(specification_str);
  EXPECT_THAT(ParseEncodingSpecification(specification_str), Not(IsOk()));
}

TEST(EncodingSpecificationParserTest, FooBarDoesNotParse) {
  CheckParserFailure("foo? bar!");
}

TEST(EncodingSpecificationParserTest, InstructionWithoutOpcodeDoesNotParse) {
  CheckParserFailure("REX.W");
  CheckParserFailure("REX.W 66");
  CheckParserFailure("REX.W /r");
  CheckParserFailure("ib");
}

TEST(EncodingSpecificationParserTest, NoPrefixAndNoSuffix) {
  CheckParser("37", "legacy_prefixes {} opcode: 0x37");
  CheckParser("0F 06", "legacy_prefixes {} opcode: 0x0f06");
}

TEST(EncodingSpecificationParserTest, NPPrefix) {
  CheckParser("NP 0F 58 /r", R"proto(
    legacy_prefixes { operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED }
    modrm_usage: FULL_MODRM
    opcode: 0x0f58)proto");
  CheckParser("NP REX.W + 0F AE /1", R"proto(
    opcode: 4014
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 1
    legacy_prefixes {
      rex_w_prefix: PREFIX_IS_REQUIRED
      operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
    })proto");
  CheckParser("NFx 0F C7 /6", R"proto(
    opcode: 0x0fc7
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 6
    legacy_prefixes {})proto");
}

TEST(EncodingSpecificationParserTest, RexPrefixAndOpcode) {
  CheckParser("REX + 80 /2 ib", R"(
                  legacy_prefixes {
                  }
                  opcode: 0x80
                  modrm_usage: OPCODE_EXTENSION_IN_MODRM
                  modrm_opcode_extension: 2
                  immediate_value_bytes: 1)");
  CheckParser("REX.W + 8B /r", R"(
                  legacy_prefixes {
                    rex_w_prefix: PREFIX_IS_REQUIRED
                  }
                  opcode: 0x8b
                  modrm_usage: FULL_MODRM)");
}

TEST(EncodingSpecificationParserTest, RexRPrefix) {
  CheckParser("REX.R + 0F 20 /0", R"proto(
    legacy_prefixes {}
    opcode: 0x0f20
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 0)proto");
}

TEST(EncodingSpecificationParserTest, MultiplePrefixes) {
  CheckParser("F2 REX 0F 38 F0 /r", R"proto(
    legacy_prefixes { has_mandatory_repne_prefix: true }
    opcode: 0x0f38f0
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, RegisterInOpcode) {
  for (const char* const specification :
       {"40+rd", "40 +rd", "40+ rd", "40 + rd", "40 +rw", "40 +rb"}) {
    CheckParser(specification, R"proto(
      legacy_prefixes {}
      opcode: 0x40
      operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE)proto");
  }
  for (const char* const specification :
       {"0F C8+rd", "0F C8 +rd", "0F C8+ rd", "0F C8 + rd"}) {
    CheckParser(specification,
                "legacy_prefixes {} opcode: 0x0fc8 "
                "operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE ");
  }
}

TEST(EncodingSpecificationParserTest, FpStackRegisterInOpcode) {
  for (const char* const specification :
       {"DD D0+i", "DD D0 +i", "DD D0+ i", "DD D0 + i"}) {
    CheckParser(specification, R"proto(
      legacy_prefixes {}
      opcode: 0xddd0
      operand_in_opcode: FP_STACK_REGISTER_IN_OPCODE)proto");
  }
}

TEST(EncodingSpecificationParserTest, ModRm) {
  CheckParser("FF /2", R"proto(
    legacy_prefixes {}
    opcode: 0xff
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 2)proto");
  CheckParser("0F AE /1", R"proto(
    legacy_prefixes {}
    opcode: 0x0FAE
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 1)proto");
  CheckParser("10 /r",
              R"proto(
                legacy_prefixes {}
                opcode: 0x10
                modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, ModRmMemorySuffix) {
  CheckParser("REX.W + 0F C7 /1 m128", R"(
                  legacy_prefixes {
                    rex_w_prefix: PREFIX_IS_REQUIRED
                  }
                  opcode: 0x0fc7
                  modrm_usage: OPCODE_EXTENSION_IN_MODRM
                  modrm_opcode_extension: 1)");
}

TEST(EncodingSpecificationParserTest, ImmediateValue) {
  CheckParser("D5 ib",
              R"proto(
                legacy_prefixes {}
                opcode: 0xd5
                immediate_value_bytes: 1)proto");
  CheckParser("15 iw",
              R"proto(
                legacy_prefixes {}
                opcode: 0x15
                immediate_value_bytes: 2)proto");
  CheckParser("15 id",
              R"proto(
                legacy_prefixes {}
                opcode: 0x15
                immediate_value_bytes: 4)proto");
  CheckParser("C8 iw ib", R"proto(
    legacy_prefixes {}
    opcode: 0xc8
    immediate_value_bytes: 2
    immediate_value_bytes: 1)proto");
}

TEST(EncodingSpecificationParserTest, ModRmAndImmediateValue) {
  CheckParser("81 /1 iw", R"proto(
    legacy_prefixes {}
    opcode: 0x81
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 1
    immediate_value_bytes: 2)proto");
  CheckParser("0F C2 /r ib", R"proto(
    legacy_prefixes {}
    opcode: 0x0fc2
    modrm_usage: FULL_MODRM
    immediate_value_bytes: 1)proto");
}

TEST(EncodingSpecificationParserTest, CodeOffset) {
  CheckParser("EB cb", "legacy_prefixes {} opcode: 0xeb code_offset_bytes: 1");
  CheckParser("E9 cw", "legacy_prefixes {} opcode: 0xe9 code_offset_bytes: 2");
  CheckParser("0F 82 cd",
              "legacy_prefixes {} opcode: 0x0f82 code_offset_bytes: 4");
  CheckParser("EA cp", "legacy_prefixes {} opcode: 0xea code_offset_bytes: 6");
}

TEST(EncodingSpecificationParserTest, MandatoryOperandSizeOverridePrefix) {
  CheckParser("66 0F 58 /r", R"proto(
    legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
    opcode: 0x0f58
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, MandatoryAddressSizeOverridePrefix) {
  CheckParser("67 A0 id", R"proto(
    legacy_prefixes { has_mandatory_address_size_override_prefix: true }
    opcode: 0xa0
    immediate_value_bytes: 4)proto");
}

TEST(EncodingSpecificationParserTest, MandatoryRepnePrefix) {
  CheckParser("F2 0F 58 /r", R"proto(
    legacy_prefixes { has_mandatory_repne_prefix: true }
    opcode: 0x0f58
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, VexNoSuffix) {
  CheckParser("VEX.128.0F.WIG 77", R"proto(
    vex_prefix {
      vector_size: VEX_VECTOR_SIZE_128_BIT
      prefix_type: VEX_PREFIX
      map_select: MAP_SELECT_0F
    }
    opcode: 0x0f77)proto");
  CheckParser("VEX.256.0F.WIG 77", R"proto(
    vex_prefix {
      vector_size: VEX_VECTOR_SIZE_256_BIT
      prefix_type: VEX_PREFIX
      map_select: MAP_SELECT_0F
    }
    opcode: 0x0f77)proto");
}

TEST(EncodingSpecificationParserTest, VexLig128) {
  CheckParser("VEX.DDS.LIG.128.66.0F38.W1 99 /r", R"proto(
    vex_prefix {
      prefix_type: VEX_PREFIX
      vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ONE
    }
    opcode: 0x0f3899
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, VexLSynonyms) {
  CheckParser("VEX.L0.0F.WIG 77", R"proto(
    vex_prefix {
      vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
      prefix_type: VEX_PREFIX
      map_select: MAP_SELECT_0F
    }
    opcode: 0x0f77)proto");
  CheckParser("VEX.L1.0F.WIG 77", R"proto(
    vex_prefix {
      vector_size: VEX_VECTOR_SIZE_BIT_IS_ONE
      prefix_type: VEX_PREFIX
      map_select: MAP_SELECT_0F
    }
    opcode: 0x0f77)proto");
}

TEST(EncodingSpecificationParserTest, Vex512) {
  // The VEX prefix does not allow 512-bit vector size. Check that the parser
  // fails if this happens.
  CheckParserFailure("VEX.DDS.512.66.0F38.W1 99 /r");
}

TEST(EncodingSpecificationParserTest, VexOperandSpecifiedInPrefix) {
  CheckParser("VEX.NDS.LZ.F3.0F38.W1 F5 /r", R"proto(
    vex_prefix {
      prefix_type: VEX_PREFIX
      vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
      vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
      mandatory_prefix: MANDATORY_PREFIX_REPE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ONE
    }
    opcode: 0x0f38f5
    modrm_usage: FULL_MODRM)proto");
  CheckParser("VEX.DDS.128.66.0F38.W1 98 /r", R"proto(
    vex_prefix {
      prefix_type: VEX_PREFIX
      vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ONE
    }
    opcode: 0x0f3898
    modrm_usage: FULL_MODRM)proto");
  CheckParser("VEX.NDD.128.66.0F.WIG 72 /6 ib", R"proto(
    vex_prefix {
      prefix_type: VEX_PREFIX
      vex_operand_usage: VEX_OPERAND_IS_DESTINATION_REGISTER
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F
      vex_w_usage: VEX_W_IS_IGNORED
    }
    opcode: 0x0f72
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 6
    immediate_value_bytes: 1)proto");
}

TEST(EncodingSpecificationParserTest, VexOperandSuffixByte) {
  CheckParser("VEX.NDS.128.66.0F3A.W0 4B /r /is4", R"proto(
    vex_prefix {
      prefix_type: VEX_PREFIX
      vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F3A
      vex_w_usage: VEX_W_IS_ZERO
      has_vex_operand_suffix: true
    }
    opcode: 0x0f3a4b
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, VSibSuffixByte) {
  CheckParser("EVEX.128.66.0F38.W0 92 /vsib", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ZERO
      vsib_usage: VSIB_USED
    }
    opcode: 0x0f3892
    modrm_usage: FULL_MODRM)proto");
  CheckParser("EVEX.128.66.0F38.W0 92 /r /vsib", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ZERO
      vsib_usage: VSIB_USED
    }
    opcode: 0x0f3892
    modrm_usage: FULL_MODRM)proto");
  CheckParser("EVEX.128.66.0F38.W0 92 /5 /vsib", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      vector_size: VEX_VECTOR_SIZE_128_BIT
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      vex_w_usage: VEX_W_IS_ZERO
      vsib_usage: VSIB_USED
    }
    opcode: 0x0f3892
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 5)proto");
}

TEST(EncodingSpecificationParserTest, EvexPrefixWithModRm) {
  CheckParser("EVEX.LIG.66.0F.W1 2F /r", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F
      vex_w_usage: VEX_W_IS_ONE
    }
    opcode: 0x0f2f
    modrm_usage: FULL_MODRM)proto");
  CheckParser("EVEX.128.0F.W0 29 /r", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      map_select: MAP_SELECT_0F
      vector_size: VEX_VECTOR_SIZE_128_BIT
      vex_w_usage: VEX_W_IS_ZERO
    }
    opcode: 0x0f29
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, EvexLig512) {
  CheckParser("EVEX.512.0F.W0 29 /r", R"proto(
    vex_prefix {
      prefix_type: EVEX_PREFIX
      map_select: MAP_SELECT_0F
      vector_size: VEX_VECTOR_SIZE_512_BIT
      vex_w_usage: VEX_W_IS_ZERO
    }
    opcode: 0x0f29
    modrm_usage: FULL_MODRM)proto");
}

TEST(EncodingSpecificationParserTest, ShadowStackPointer) {
  CheckParser("F3 REX.W 0F AE /05", R"proto(
    opcode: 0x0fae
    modrm_usage: OPCODE_EXTENSION_IN_MODRM
    modrm_opcode_extension: 5
    legacy_prefixes {
      has_mandatory_repe_prefix: true
      rex_w_prefix: PREFIX_IS_REQUIRED
    })proto");
}

TEST(GetAvailableEncodingsTest, GetEncodings) {
  static const struct {
    const char* encoding_specification;
    std::initializer_list<InstructionOperand::Encoding>
        expected_available_encodings;
  } kTestCases[] = {{"VEX.NDS.LZ.F3.0F38.W1 F5 /r",
                     {InstructionOperand::MODRM_REG_ENCODING,
                      InstructionOperand::MODRM_RM_ENCODING,
                      InstructionOperand::VEX_V_ENCODING}},
                    {"REX + 80 /2 ib",
                     {InstructionOperand::IMMEDIATE_VALUE_ENCODING,
                      InstructionOperand::MODRM_RM_ENCODING}},
                    {"40 + rd", {InstructionOperand::OPCODE_ENCODING}},
                    {"C8 iw ib",
                     {InstructionOperand::IMMEDIATE_VALUE_ENCODING,
                      InstructionOperand::IMMEDIATE_VALUE_ENCODING}},
                    {"VEX.NDS.128.66.0F3A.W0 4B /r /is4",
                     {InstructionOperand::MODRM_REG_ENCODING,
                      InstructionOperand::MODRM_RM_ENCODING,
                      InstructionOperand::VEX_SUFFIX_ENCODING,
                      InstructionOperand::VEX_V_ENCODING}},
                    {"EVEX.128.66.0F38.W0 92 /vsib",
                     {InstructionOperand::VSIB_ENCODING,
                      InstructionOperand::MODRM_REG_ENCODING,
                      InstructionOperand::VEX_V_ENCODING}},
                    {"EVEX.512.66.0F38.W0 C6 /6 /vsib",
                     {InstructionOperand::VSIB_ENCODING,
                      InstructionOperand::VEX_V_ENCODING}}};
  for (const auto& test_case : kTestCases) {
    EXPECT_THAT(
        ParseEncodingSpecification(test_case.encoding_specification),
        IsOkAndHolds(ResultOf(GetAvailableEncodings,
                              UnorderedElementsAreArray(
                                  test_case.expected_available_encodings))));
  }
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
