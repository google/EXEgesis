// Copyright 2018 Google Inc.
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

#include "exegesis/x86/instruction_encoder.h"

#include <cstdint>

#include "absl/memory/memory.h"
#include "absl/types/span.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/x86/architecture.h"
#include "exegesis/x86/instruction_encoding_test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::IsOk;
using ::exegesis::util::StatusOr;
using ::testing::ElementsAreArray;
using ::testing::Not;

constexpr char kArchitectureProto[] = R"proto(
  instruction_set {
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '0F 06'
      x86_encoding_specification {
        opcode: 3846
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '66 0F 3A 09 /r ib'
      x86_encoding_specification {
        opcode: 997897
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        immediate_value_bytes: 1
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '87 /r'
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '98'
      x86_encoding_specification {
        opcode: 152
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '0F C8+rd'
      x86_encoding_specification {
        opcode: 4040
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '14 ib'
      x86_encoding_specification {
        opcode: 20
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
        immediate_value_bytes: 1
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'C8 iw ib'
      x86_encoding_specification {
        opcode: 200
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
        immediate_value_bytes: 2
        immediate_value_bytes: 1
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'E8 cd'
      x86_encoding_specification {
        opcode: 232
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
        code_offset_bytes: 4
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: '90+rd'
      x86_encoding_specification {
        opcode: 144
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: false
      raw_encoding_specification: 'REX.W + 87 /r'
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: false
      raw_encoding_specification: '66 87 /r'
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'FF /1'
      x86_encoding_specification {
        opcode: 255
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 1
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'C7 /0 id'
      x86_encoding_specification {
        opcode: 199
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 4
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'VEX.256.66.0F38.W0 0E /r'
      x86_encoding_specification {
        opcode: 997390
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_256_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'VEX.256.0F.WIG 77'
      x86_encoding_specification {
        opcode: 3959
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_256_BIT
          map_select: MAP_SELECT_0F
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'VEX.DDS.LIG.66.0F38.W0 9F /r'
      x86_encoding_specification {
        opcode: 997535
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'VEX.NDS.128.66.0F3A.W0 4B /r /is4'
      x86_encoding_specification {
        opcode: 997963
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F3A
          vex_w_usage: VEX_W_IS_ZERO
          has_vex_operand_suffix: true
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.128.F3.0F.W0 E6 /r'
      x86_encoding_specification {
        opcode: 4070
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_REPE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_32_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.DDS.LIG.66.0F38.W0 9F /r'
      x86_encoding_specification {
        opcode: 997535
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.NDS.128.66.0F38.W1 AE /r'
      x86_encoding_specification {
        opcode: 997550
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.NDS.512.66.0F38.W1 AE /r'
      x86_encoding_specification {
        opcode: 997550
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_512_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.NDS.128.0F.W0 58 /r'
      x86_encoding_specification {
        opcode: 3928
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_32_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: 'EVEX.128.66.0F3A.W1 66 /r ib'
      x86_encoding_specification {
        opcode: 997990
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F3A
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_ONLY
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
        immediate_value_bytes: 1
      }
    }
  })proto";

class EncodeInstructionTest : public ::testing::Test {
 protected:
  // Tests the instruction encoder: assumes that 'specification_str' is the
  // encoding specification in the language used in the Intel manual,
  // 'decoded_instruction_str' is an DecodedInstruction proto in the text
  // format, 'expected_encoding' contains the expected binary encoding of the
  // instruction, and 'expected_instruction_format_str' is an InstructionFormat
  // proto in the text format that contains the expected disassembly of the
  // instruction in the Intel assembly syntax.
  // The method encodes 'decoded_instruction_str' using 'specification_str' as
  // the encoding specification. Verifies that the encoder succeeds, the binary
  // code is equal to 'expected_encoding', and it disassembles to
  // 'expected_instruction_format_str'.
  void TestInstructionEncoder(
      const std::string& specification_str,
      const std::string& decoded_instruction_str,
      absl::Span<const uint8_t> expected_encoding,
      const std::string& expected_instruction_format_str);

  // Tests that the instruction encoder fails on invalid input. Assumes that
  // 'specification_str' is the encoding specification in the language used in
  // the Intel manual and 'decoded_instruction_str' is an DecodedInstruction
  // proto in the text format. Runs the instruction encoder with these two
  // inputs, and verifies that it returns an error status.
  void TestInstructionEncoderFailure(
      const std::string& specification_str,
      const std::string& decoded_instruction_str);

  void SetUp() override {
    auto architecture_proto = std::make_shared<ArchitectureProto>();
    ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
        kArchitectureProto, architecture_proto.get()));
    architecture_ =
        absl::make_unique<X86Architecture>(std::move(architecture_proto));
  }

  std::unique_ptr<X86Architecture> architecture_;
};

void EncodeInstructionTest::TestInstructionEncoder(
    const std::string& specification_str,
    const std::string& decoded_instruction_str,
    absl::Span<const uint8_t> expected_encoding,
    const std::string& expected_instruction_format_str) {
  const std::vector<X86Architecture::InstructionIndex> instruction_indices =
      architecture_->GetInstructionIndicesByRawEncodingSpecification(
          specification_str);
  ASSERT_GE(instruction_indices.size(), 1);
  const EncodingSpecification& specification =
      architecture_->encoding_specification(instruction_indices.front());
  DecodedInstruction decoded_instruction;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      decoded_instruction_str, &decoded_instruction));

  const StatusOr<std::vector<uint8_t>> encoded_instruction_or_status =
      EncodeInstruction(specification, decoded_instruction);
  ASSERT_OK(encoded_instruction_or_status.status());
  EXPECT_THAT(encoded_instruction_or_status.ValueOrDie(),
              ElementsAreArray(expected_encoding));
  EXPECT_THAT(decoded_instruction,
              DisassemblesTo(specification, expected_instruction_format_str));
}

// Tests that the instruction encoder fails on invalid input. Assumes that
// 'specification_str' is the encoding specification in the language used in
// the Intel manual and 'decoded_instruction_str' is an DecodedInstruction
// proto in the text format. Runs the instruction encoder with these two
// inputs, and verifies that it returns an error status.
void EncodeInstructionTest::TestInstructionEncoderFailure(
    const std::string& specification_str,
    const std::string& decoded_instruction_str) {
  const std::vector<X86Architecture::InstructionIndex> instruction_indices =
      architecture_->GetInstructionIndicesByRawEncodingSpecification(
          specification_str);
  ASSERT_GE(instruction_indices.size(), 1);
  const EncodingSpecification& specification =
      architecture_->encoding_specification(instruction_indices.front());
  DecodedInstruction decoded_instruction;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      decoded_instruction_str, &decoded_instruction));

  EXPECT_THAT(EncodeInstruction(specification, decoded_instruction),
              Not(IsOk()));
}

TEST_F(EncodeInstructionTest, OpcodeMismatch) {
  constexpr char kSpecification[] = "0F 06";
  constexpr char kDecodedInstruction[] = "opcode: 0x98";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, MissingModRm) {
  constexpr char kSpecification[] = "66 0F 3A 09 /r ib";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    immediate_value: '\x00')proto";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, MissingImmediateValue) {
  constexpr char kSpecification[] = "66 0F 3A 09 /r ib";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 2 })proto";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, MissingSib) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm { addressing_mode: INDIRECT register_operand: 2 rm_operand: 4 })proto";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, JustOneByteOpcode) {
  constexpr char kSpecification[] = "98";
  constexpr char kDecodedInstruction[] = "";
  constexpr uint8_t kExpectedEncoding[] = {0x98};
  constexpr char kExpectedDisassembly[] = "CWDE";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, JustTwoByteOpcode) {
  constexpr char kSpecification[] = "0F 06";
  constexpr char kDecodedInstruction[] = "opcode: 0x0f06";
  constexpr uint8_t kExpectedEncoding[] = {0x0f, 0x06};
  constexpr char kExpectedDisassembly[] = "CLTS";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, ThreeByteOpcodeAndStuff) {
  // There is no operand-less instruction with a three-byte opcode.
  constexpr char kSpecification[] = "66 0F 3A 09 /r ib";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 2 }
    immediate_value: '\x00')proto";
  constexpr uint8_t kExpectedEncoding[] = {0x66, 0x0f, 0x3a, 0x09, 0xca, 0x00};
  constexpr char kExpectedDisassembly[] = "ROUNDPD XMM1, XMM2, 0x0";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OperandInOpcode) {
  constexpr char kSpecification[] = "0F C8+rd";
  constexpr char kDecodedInstruction[] = "opcode: 0x0fc9";
  constexpr uint8_t kExpectedEncoding[] = {0x0f, 0xc9};
  constexpr char kExpectedDisassembly[] = "BSWAP ECX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OperandInOpcode_DefaultOpcode) {
  // The opcode in the specification has all three least significant bits set to
  // zero - using it directly means using register index 0 (EAX) for the operand
  // in the opcode.
  constexpr char kSpecification[] = "0F C8+rd";
  constexpr char kDecodedInstruction[] = "";
  constexpr uint8_t kExpectedEncoding[] = {0x0f, 0xc8};
  constexpr char kExpectedDisassembly[] = "BSWAP EAX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OperandInOpcode_InvalidOpcode) {
  constexpr char kSpecification[] = "0F C8+rd";
  constexpr char kDecodedInstruction[] = "opcode: 0x0fc7";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, OpcodeAndImmediateValue) {
  constexpr char kSpecification[] = "14 ib";
  constexpr char kDecodedInstruction[] = R"proto(immediate_value: '\x25')proto";
  constexpr uint8_t kExpectedEncoding[] = {0x14, 0x25};
  constexpr char kExpectedDisassembly[] = "ADC AL, 0x25";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndTwoImmediateValues) {
  constexpr char kSpecification[] = "C8 iw ib";
  constexpr char kDecodedInstruction[] = R"proto(
    immediate_value: '\x25\x26'
    immediate_value: '\x01')proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc8, 0x25, 0x26, 0x01};
  constexpr char kExpectedDisassembly[] = "ENTER 0x2625, 0x1";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndCodeOffset) {
  constexpr char kSpecification[] = "E8 cd";
  constexpr char kDecodedInstruction[] =
      R"proto(code_offset: '\xab\xcd\xef\x01')proto";
  constexpr uint8_t kExpectedEncoding[] = {0xe8, 0xab, 0xcd, 0xef, 0x01};
  constexpr char kExpectedDisassembly[] = "CALL 0x1efcdab";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndModRm) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm { addressing_mode: DIRECT register_operand: 2 rm_operand: 5 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0xd5};
  constexpr char kExpectedDisassembly[] = "XCHG EDX, EBP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndModRmWithSib) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm { addressing_mode: INDIRECT register_operand: 2 rm_operand: 4 }
    sib { base: 6 index: 1 scale: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0x14, 0x8e};
  constexpr char kExpectedDisassembly[] = "XCHG DWORD PTR [RSI + 4*RCX], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndModRmWithSibAnd8BitDisplacement) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      register_operand: 2
      rm_operand: 4
      address_displacement: 64
    }
    sib { base: 6 index: 1 scale: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0x54, 0x8e, 0x40};
  constexpr char kExpectedDisassembly[] =
      "XCHG DWORD PTR [RSI + 4*RCX + 0x40], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest,
       OpcodeAndModRmWithSibAndNegative8BitDisplacement) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      register_operand: 2
      rm_operand: 4
      address_displacement: -32
    }
    sib { base: 6 index: 1 scale: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0x54, 0x8e, 0xe0};
  constexpr char kExpectedDisassembly[] =
      "XCHG DWORD PTR [RSI + 4*RCX - 0x20], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndModRmWithSibAnd32BitDisplacement) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm {
      addressing_mode: INDIRECT_WITH_32_BIT_DISPLACEMENT
      register_operand: 2
      rm_operand: 4
      address_displacement: 0x12345678
    }
    sib { base: 6 index: 1 scale: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0x94, 0x8e, 0x78,
                                           0x56, 0x34, 0x12};
  constexpr char kExpectedDisassembly[] =
      "XCHG DWORD PTR [RSI + 4*RCX + 0x12345678], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeAndModRmWithRipRelativeAddressing) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm {
      addressing_mode: INDIRECT
      register_operand: 2
      rm_operand: 5
      address_displacement: -78
    })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x87, 0x15, 0xb2, 0xff, 0xff, 0xff};
  constexpr char kExpectedDisassembly[] = "XCHG DWORD PTR [RIP - 0x4e], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest,
       AddressSizeOverrideOpcodeAndModRmWithSibAnd32BitDisplacement) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    address_size_override: ADDRESS_SIZE_OVERRIDE
    modrm {
      addressing_mode: INDIRECT_WITH_32_BIT_DISPLACEMENT
      register_operand: 2
      rm_operand: 4
      address_displacement: 0x12345678
    }
    sib { base: 6 index: 1 scale: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x67, 0x87, 0x94, 0x8e,
                                           0x78, 0x56, 0x34, 0x12};
  constexpr char kExpectedDisassembly[] =
      "XCHG DWORD PTR [ESI + 4*ECX + 0x12345678], EDX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, SegmentOverridePrefix) {
  constexpr char kSpecification[] = "90+rd";
  constexpr char kDecodedInstruction[] = R"proto(
    segment_override: SS_OVERRIDE)proto";
  constexpr uint8_t kExpectedEncoding[] = {0x36, 0x90};
  constexpr char kExpectedDisassembly[] = "NOP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OperandSizeOverrideOpcodeAndModRm) {
  constexpr char kSpecification[] = "66 87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    modrm { addressing_mode: DIRECT register_operand: 2 rm_operand: 5 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x66, 0x87, 0xd5};
  constexpr char kExpectedDisassembly[] = "XCHG DX, BP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, RexWOpcodeAndModRm) {
  constexpr char kSpecification[] = "REX.W + 87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { rex { w: true } }
    modrm { addressing_mode: DIRECT register_operand: 2 rm_operand: 5 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x48, 0x87, 0xd5};
  constexpr char kExpectedDisassembly[] = "XCHG RDX, RBP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, RexROpcodeAndModRm) {
  constexpr char kSpecification[] = "87 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    legacy_prefixes { rex { r: true } }
    modrm { addressing_mode: DIRECT register_operand: 2 rm_operand: 5 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x44, 0x87, 0xd5};
  constexpr char kExpectedDisassembly[] = "XCHG R10D, EBP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, RexRWOpcodeAndModRm) {
  constexpr char kSpecification[] = "REX.W + 87 /r";
  constexpr char kDecodedInstruction[] = R"(
      legacy_prefixes {
        rex {
          r: true
          w: true
        }
      }
      modrm {
        addressing_mode: DIRECT
        register_operand: 2
        rm_operand: 5
      })";
  constexpr uint8_t kExpectedEncoding[] = {0x4c, 0x87, 0xd5};
  constexpr char kExpectedDisassembly[] = "XCHG R10, RBP";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeInModRm) {
  constexpr char kSpecification[] = "FF /1";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 1 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0xff, 0xc9};
  constexpr char kExpectedDisassembly[] = "DEC ECX";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, OpcodeInModRmAndImmediate) {
  constexpr char kSpecification[] = "C7 /0 id";
  constexpr char kDecodedInstruction[] = R"proto(
    modrm { addressing_mode: DIRECT register_operand: 0 rm_operand: 5 }
    immediate_value: '\x78\x56\x34\x12')proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc7, 0xc5, 0x78, 0x56, 0x34, 0x12};
  constexpr char kExpectedDisassembly[] = "MOV EBP, 0x12345678";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, VexWithNoOperandInPrefix) {
  constexpr char kSpecification[] = "VEX.256.66.0F38.W0 0E /r";
  constexpr char kDecodedInstruction[] = R"proto(
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      use_256_bit_vector_length: true
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
    }
    modrm { addressing_mode: DIRECT register_operand: 0 rm_operand: 5 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc4, 0xe2, 0x7d, 0x0e, 0xc5};
  constexpr char kExpectedDisassembly[] = "VTESTPS YMM0, YMM5";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, TwoByteVexPrefix) {
  constexpr char kSpecification[] = "VEX.256.0F.WIG 77";
  constexpr char kDecodedInstruction[] = R"proto(
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      mandatory_prefix: NO_MANDATORY_PREFIX
      use_256_bit_vector_length: true
      map_select: MAP_SELECT_0F
    })proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc5, 0xfc, 0x77};
  constexpr char kExpectedDisassembly[] = "VZEROALL";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, TwoByteVexPrefixAndSegmentOverride) {
  constexpr char kSpecification[] = "VEX.256.0F.WIG 77";
  constexpr char kDecodedInstruction[] = R"proto(
    segment_override: FS_OVERRIDE
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      mandatory_prefix: NO_MANDATORY_PREFIX
      use_256_bit_vector_length: true
      map_select: MAP_SELECT_0F
    })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x64, 0xc5, 0xfc, 0x77};
  constexpr char kExpectedDisassembly[] = "VZEROALL";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, TwoByteVexPrefixAndAddressSizeOverride) {
  constexpr char kSpecification[] = "VEX.256.0F.WIG 77";
  constexpr char kDecodedInstruction[] = R"proto(
    address_size_override: ADDRESS_SIZE_OVERRIDE
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      mandatory_prefix: NO_MANDATORY_PREFIX
      use_256_bit_vector_length: true
      map_select: MAP_SELECT_0F
    })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x67, 0xc5, 0xfc, 0x77};
  constexpr char kExpectedDisassembly[] = "VZEROALL";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, VexWithOperandInPrefix) {
  constexpr char kSpecification[] = "VEX.DDS.LIG.66.0F38.W0 9F /r";
  constexpr char kDecodedInstruction[] = R"proto(
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      w: false
      inverted_register_operand: 8
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
    }
    modrm { addressing_mode: DIRECT register_operand: 3 rm_operand: 4 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc4, 0xe2, 0x41, 0x9f, 0xdc};
  constexpr char kExpectedDisassembly[] = "VFNMSUB132SS XMM3, XMM7, XMM4";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, VexWithOperandInSuffix) {
  constexpr char kSpecification[] = "VEX.NDS.128.66.0F3A.W0 4B /r /is4";
  constexpr char kDecodedInstruction[] = R"proto(
    vex_prefix {
      not_b: true
      not_r: true
      not_x: true
      w: false
      inverted_register_operand: 13
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F3A
      vex_suffix_value: 0x40
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 3 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0xc4, 0xe3, 0x69, 0x4b, 0xcb, 0x40};
  constexpr char kExpectedDisassembly[] = "VBLENDVPD XMM1, XMM2, XMM3, XMM4";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexWithNoOperandInPrefix) {
  constexpr char kSpecification[] = "EVEX.128.F3.0F.W0 E6 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_REPE
      map_select: MAP_SELECT_0F
      opmask_register: 1
      not_b: true
      not_r: 3
      not_x: true
      inverted_register_operand: 31
      z: true
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x62, 0xf1, 0x7e, 0x89, 0xe6, 0xca};
  constexpr char kExpectedDisassembly[] = "VCVTDQ2PD XMM1 {k1} {z}, XMM2";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexWithOperandInPrefix) {
  constexpr char kSpecification[] = "EVEX.DDS.LIG.66.0F38.W0 9F /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      not_b: true
      not_r: 3
      not_x: true
      inverted_register_operand: 11
      z: true
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x62, 0xf2, 0x5d, 0x80, 0x9f, 0xca};
  constexpr char kExpectedDisassembly[] =
      "VFNMSUB132SS XMM1 {k0} {z}, XMM20, XMM2";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexWithExtendedVectorOperand) {
  constexpr char kSpecification[] = "EVEX.128.F3.0F.W0 E6 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_REPE
      map_select: MAP_SELECT_0F
      opmask_register: 1
      not_b: true
      not_r: 1
      not_x: true
      inverted_register_operand: 31
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 2 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x62, 0xe1, 0x7e, 0x09, 0xe6, 0xca};
  constexpr char kExpectedDisassembly[] = "VCVTDQ2PD XMM17 {k1}, XMM2";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexWithMemoryOperandAndBroadcast) {
  constexpr char kSpecification[] = "EVEX.NDS.128.66.0F38.W1 AE /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      w: true
      not_b: true
      not_r: 3
      not_x: true
      broadcast_or_control: true
      inverted_register_operand: 31
    }
    modrm { addressing_mode: INDIRECT register_operand: 1 rm_operand: 3 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x62, 0xf2, 0xfd, 0x18, 0xae, 0x0b};
  constexpr char kExpectedDisassembly[] =
      "VFNMSUB213PD XMM1, XMM0, QWORD PTR [RBX] {1to2}";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexWith512BitOperands) {
  constexpr char kSpecification[] = "EVEX.NDS.512.66.0F38.W1 AE /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      map_select: MAP_SELECT_0F38
      w: true
      not_b: true
      not_r: 3
      not_x: true
      vector_length_or_rounding: 2
      opmask_register: 1
      inverted_register_operand: 31
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 3 })proto";
  constexpr uint8_t kExpectedEncoding[] = {0x62, 0xf2, 0xfd, 0x49, 0xae, 0xcb};
  constexpr char kExpectedDisassembly[] = "VFNMSUB213PD ZMM1 {k1}, ZMM0, ZMM3";
  TestInstructionEncoder(kSpecification, kDecodedInstruction, kExpectedEncoding,
                         kExpectedDisassembly);
}

TEST_F(EncodeInstructionTest, EvexBroadcastNotSupported) {
  // VADDPS xmm1, xmm2, xmm3; the instruction supports broadcast only with
  // indirect addressing mode.
  constexpr char kSpecification[] = "EVEX.NDS.128.0F.W0 58 /r";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      map_select: MAP_SELECT_0F
      opmask_register: 1
      not_b: true
      not_r: 3
      not_x: true
      broadcast_or_control: true
      inverted_register_operand: 31
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 3 })proto";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

TEST_F(EncodeInstructionTest, EvexZeroingNotSupported) {
  // VFPCLASSPD supports only merging masking, but not zeroing.
  constexpr char kSpecification[] = "EVEX.128.66.0F3A.W1 66 /r ib";
  constexpr char kDecodedInstruction[] = R"proto(
    evex_prefix {
      map_select: MAP_SELECT_0F3A
      opmask_register: 1
      w: true
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      not_b: true
      not_r: 3
      not_x: true
      z: true
    }
    modrm { addressing_mode: DIRECT register_operand: 1 rm_operand: 3 }
    immediate_value: '\x01')proto";
  TestInstructionEncoderFailure(kSpecification, kDecodedInstruction);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
