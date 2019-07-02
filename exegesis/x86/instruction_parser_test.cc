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

// Contains the tests for the parser of the x86-64 binary instruction encoding.
// TODO(ondrasej): Use the LLVM assembler/disassembler to verify the parses.

#include "exegesis/x86/instruction_parser.h"

#include <cstdint>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/x86/decoded_instruction.pb.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/strings.h"
#include "exegesis/x86/instruction_encoder.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;
using ::exegesis::util::error::Code;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::exegesis::util::error::NOT_FOUND;
using ::testing::HasSubstr;
using ::testing::UnorderedElementsAreArray;

class InstructionParserTest : public ::testing::Test {
 protected:
  static const char* const kArchitectureProto;

  void ParseInstructionAndCheckResult(
      absl::Span<const uint8_t> binary_encoding,
      const std::string& encoding_specification_str,
      const std::string& expected_encoded_instruction_proto);
  void ParseInstructionAndCheckError(absl::Span<const uint8_t> binary_encoding,
                                     Code expected_error_code,
                                     const std::string& expected_error_message);

  void SetUp() override {
    auto architecture_proto = std::make_shared<ArchitectureProto>();
    ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
        kArchitectureProto, architecture_proto.get()));
    architecture_ = absl::make_unique<X86Architecture>(architecture_proto);
  }

  std::unique_ptr<X86Architecture> architecture_;
};

const char* const InstructionParserTest::kArchitectureProto = R"proto(
  instruction_set {
    instructions {
      vendor_syntax { mnemonic: "FCOS" }
      raw_encoding_specification: "D9 FF"
      x86_encoding_specification {
        opcode: 0xD9FF
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "FLD" }
      raw_encoding_specification: "D9 /0"
      x86_encoding_specification {
        opcode: 0xD9
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "FADD" }
      raw_encoding_specification: "D8 /0"
      x86_encoding_specification {
        opcode: 0xD8
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "FSUB" }
      raw_encoding_specification: "D8 /4"
      x86_encoding_specification {
        opcode: 0xD8
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "FADD" }
      raw_encoding_specification: "DC /0"
      x86_encoding_specification {
        opcode: 0xDC
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "BSWAP"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
        }
      }
      raw_encoding_specification: "0F C8+rd"
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
      vendor_syntax {
        mnemonic: "MOV"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: OPCODE_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 64
          name: "imm64"
          usage: USAGE_READ
        }
      }
      available_in_64_bit: true
      encoding_scheme: "OI"
      raw_encoding_specification: "REX.W + B8+ rd io"
      x86_encoding_specification {
        opcode: 184
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 8
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "POP"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
          name: "m64"
          usage: USAGE_WRITE
        }
      }
      raw_encoding_specification: "8F /0"
      x86_encoding_specification {
        opcode: 143
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'ADC' }
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
      vendor_syntax { mnemonic: 'ADC' }
      raw_encoding_specification: '66 15 iw'
      x86_encoding_specification {
        opcode: 21
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED

        }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'ADC' }
      raw_encoding_specification: '15 id'
      x86_encoding_specification {
        opcode: 21
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'ANDN' }
      raw_encoding_specification: 'VEX.NDS.LZ. 0F38.W1 F2 /r'
      x86_encoding_specification {
        opcode: 997618
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'CDQE' }
      raw_encoding_specification: 'REX.W + 98'
      x86_encoding_specification {
        opcode: 152
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'CRC32' }
      raw_encoding_specification: 'F2 0F 38 F1 /r'
      x86_encoding_specification {
        opcode: 997617
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
          has_mandatory_repne_prefix: true
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'CWDE' }
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
      vendor_syntax { mnemonic: 'ENTER' }
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
      vendor_syntax { mnemonic: 'INVD' }
      raw_encoding_specification: '0F 08'
      protection_mode: 0
      x86_encoding_specification {
        opcode: 3848
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'MOV' }
      raw_encoding_specification: '8B /r'
      x86_encoding_specification {
        opcode: 139
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'MOVBE' }
      raw_encoding_specification: '0F 38 F1 /r'
      x86_encoding_specification {
        opcode: 997617
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'NOP' }
      raw_encoding_specification: 'NP 90'
      x86_encoding_specification {
        opcode: 144
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'PEXT' }
      raw_encoding_specification: 'VEX.NDS.LZ.F3.0F38.W0 F5 /r'
      x86_encoding_specification {
        opcode: 997621
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          mandatory_prefix: MANDATORY_PREFIX_REPE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'PEXT' }
      raw_encoding_specification: 'VEX.NDS.LZ.F3.0F38.W1 F5 /r'
      x86_encoding_specification {
        opcode: 997621
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          mandatory_prefix: MANDATORY_PREFIX_REPE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'SHRX' }
      raw_encoding_specification: 'VEX.NDS.LZ.F2.0F38.W1 F7 /r'
      x86_encoding_specification {
        opcode: 997623
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'VADDPD' }
      raw_encoding_specification: 'VEX.NDS.128.66.0F.WIG 58 /r'
      x86_encoding_specification {
        opcode: 3928
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'VADDPD' }
      raw_encoding_specification: 'VEX.NDS.256.66.0F.WIG 58 /r'
      x86_encoding_specification {
        opcode: 3928
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_256_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'VBLENDPD' }
      raw_encoding_specification: 'VEX.NDS.128.66.0F3A.WIG 0D /r ib'
      x86_encoding_specification {
        opcode: 997901
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F3A
        }
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'VBLENDVPD' }
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
      vendor_syntax { mnemonic: 'VMOVSD' }
      raw_encoding_specification: 'EVEX.NDS.LIG.F2.0F.W1 10 /r'
      x86_encoding_specification {
        opcode: 3856
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ONE
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'XCHG' }
      raw_encoding_specification: '66 90+rw'
      x86_encoding_specification {
        opcode: 144
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED

        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'XCHG' }
      raw_encoding_specification: '90+rd'
      x86_encoding_specification {
        opcode: 144
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: 'XCHG' }
      raw_encoding_specification: '66 90+rd'
      x86_encoding_specification {
        opcode: 144
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "INVLPG"
        operands { name: "m" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "M"
      raw_encoding_specification: "0F 01/7"
      x86_encoding_specification {
        opcode: 3841
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 7
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      description: "Specifies the end of an RTM code region."
      llvm_mnemonic: "XEND"
      vendor_syntax { mnemonic: "XEND" }
      syntax { mnemonic: "xend" }
      att_syntax { mnemonic: "xend" }
      feature_name: "RTM"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "A"
      raw_encoding_specification: "NP 0F 01 D5"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 983509
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
      instruction_group_index: 624
    }
    instructions {
      description: "Store effective address for m in register r64."
      llvm_mnemonic: "LEA64r"
      vendor_syntax {
        mnemonic: "LEA"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_64_BIT
        }
        operands {
          addressing_mode: LOAD_EFFECTIVE_ADDRESS
          encoding: MODRM_RM_ENCODING
          name: "m"
          usage: USAGE_READ
        }
      }
      available_in_64_bit: true
      encoding_scheme: "RM"
      raw_encoding_specification: "REX.W + 8D /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 141
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
      instruction_group_index: 198
    }
  })proto";

void InstructionParserTest::ParseInstructionAndCheckResult(
    absl::Span<const uint8_t> binary_encoding,
    const std::string& encoding_specification_str,
    const std::string& expected_encoded_instruction_proto) {
  SCOPED_TRACE(absl::StrCat("encoding_specification_str = ",
                            encoding_specification_str));
  SCOPED_TRACE(absl::StrCat("binary_encoding = ",
                            ToHumanReadableHexString(binary_encoding)));
  // Check that the test inputs were valid to begin with: use the instruction
  // encoder to encode expected_encoded_instruction_proto with
  // encoding_specification_str, and verify that the binary encoding is the same
  // as binary_encoding.
  DecodedInstruction expected_decoded_instruction;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
      expected_encoded_instruction_proto, &expected_decoded_instruction));
  const std::vector<X86Architecture::InstructionIndex> indices =
      architecture_->GetInstructionIndicesByRawEncodingSpecification(
          encoding_specification_str);
  ASSERT_FALSE(indices.empty());
  const EncodingSpecification& encoding_specification =
      architecture_->encoding_specification(indices.front());
  const StatusOr<std::vector<uint8_t>> exegesis_binary_encoding_or_status =
      EncodeInstruction(encoding_specification, expected_decoded_instruction);
  ASSERT_OK(exegesis_binary_encoding_or_status.status());
  const std::vector<uint8_t>& exegesis_binary_encoding =
      exegesis_binary_encoding_or_status.ValueOrDie();
  // The following line compares the bytes in the binary encoding with the
  // ordering ignored. Ideally, we'd like to compare the bytes including the
  // ordering, but the encoding specification does not prescribe a fixed order
  // of the legacy prefix bytes, and using strict comparisons of the order
  // would cause failures here. We compensate for this weaker test by parsing
  // both the binary encoding from the input and the binary encoding from the
  // instruction encoder.
  EXPECT_THAT(binary_encoding,
              UnorderedElementsAreArray(exegesis_binary_encoding));
  InstructionParser instruction_parser(architecture_.get());
  // Test the instruction parser using the provided test inputs: decode the
  // instruction and check it against the expected encoded instruction proto.
  EXPECT_THAT(instruction_parser.ConsumeBinaryEncoding(&binary_encoding),
              IsOkAndHolds(EqualsProto(expected_decoded_instruction)));
  EXPECT_TRUE(binary_encoding.empty())
      << "The parser did not consume the whole input. Remaining bytes: "
      << ToHumanReadableHexString(binary_encoding);

  // Test the instruction parser using the binary encoding produced by our own
  // instruction encoder. The output of the parser should match exactly the
  // proto we used for encoding the instruction.
  absl::Span<const uint8_t> exegesis_binary_encoding_slice(
      exegesis_binary_encoding);
  EXPECT_THAT(
      instruction_parser.ConsumeBinaryEncoding(&exegesis_binary_encoding_slice),
      IsOkAndHolds(EqualsProto(expected_decoded_instruction)));
  EXPECT_TRUE(exegesis_binary_encoding_slice.empty())
      << "The parser did not consume the whole input. Remaining bytes: "
      << ToHumanReadableHexString(exegesis_binary_encoding);
}

void InstructionParserTest::ParseInstructionAndCheckError(
    absl::Span<const uint8_t> binary_encoding, Code expected_error_code,
    const std::string& expected_error_message) {
  InstructionParser instruction_parser(architecture_.get());
  EXPECT_THAT(instruction_parser.ParseBinaryEncoding(binary_encoding),
              StatusIs(expected_error_code, HasSubstr(expected_error_message)));
}

TEST_F(InstructionParserTest, ParseNop) {
  ParseInstructionAndCheckResult({0x90}, "90+rd", "opcode: 0x90");
}

TEST_F(InstructionParserTest, ParseNopWithLockPrefix) {
  ParseInstructionAndCheckResult(
      {0xf0, 0x90}, "90+rd",
      "legacy_prefixes { lock_or_rep: LOCK_PREFIX } opcode: 0x90");
}

TEST_F(InstructionParserTest, ParseNopWithRepNePrefix) {
  ParseInstructionAndCheckResult(
      {0xf2, 0x90}, "90+rd",
      "legacy_prefixes { lock_or_rep: REPNE_PREFIX } opcode: 0x90");
}

TEST_F(InstructionParserTest, ParseNopWithRepPrefix) {
  ParseInstructionAndCheckResult(
      {0xf3, 0x90}, "90+rd",
      "legacy_prefixes { lock_or_rep: REP_PREFIX } opcode: 0x90");
}

TEST_F(InstructionParserTest, ParseNopWithOperandSizeOverride) {
  ParseInstructionAndCheckResult({0x66, 0x90}, "66 90+rd", R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    opcode: 0x90)proto");
  // Check that a repeated operand size override is parsed correctly. Repeated
  // prefixes are discouraged by Intel (not officially supported), but they seem
  // to work just fine on the CPUs, and seem to be emitted by GCC in some cases.
  // We can't use ParseInstructionAndCheckResult here, because the re-encoding
  // of the instruction would have only one operand size override prefix, and
  // wouldn't match the parsed encoding.
  InstructionParser instruction_parser(architecture_.get());
  absl::Span<const uint8_t> binary_encoding = {0x66, 0x66, 0x90};
  EXPECT_THAT(instruction_parser.ConsumeBinaryEncoding(&binary_encoding),
              IsOkAndHolds(EqualsProto(R"proto(
                legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
                opcode: 0x90)proto")));
  EXPECT_TRUE(binary_encoding.empty())
      << "The parser did not consume the whole input. Remaining bytes: "
      << ToHumanReadableHexString(binary_encoding);
}

TEST_F(InstructionParserTest, ParseNopWithAddressSizeOverride) {
  ParseInstructionAndCheckResult({0x67, 0x90}, "90+rd", R"proto(
    address_size_override: ADDRESS_SIZE_OVERRIDE
    opcode: 0x90)proto");
  // Check that a repeated operand size override is parsed correctly. Repeated
  // prefixes are discouraged by Intel (not officially supported), but they seem
  // to work just fine on the CPUs, and seem to be emitted by GCC in some cases.
  // We can't use ParseInstructionAndCheckResult here, because the re-encoding
  // of the instruction would have only one operand size override prefix, and
  // wouldn't match the parsed encoding.
  InstructionParser instruction_parser(architecture_.get());
  absl::Span<const uint8_t> binary_encoding = {0x67, 0x67, 0x90};
  EXPECT_THAT(instruction_parser.ConsumeBinaryEncoding(&binary_encoding),
              IsOkAndHolds(EqualsProto(R"proto(
                address_size_override: ADDRESS_SIZE_OVERRIDE
                opcode: 0x90)proto")));
  EXPECT_TRUE(binary_encoding.empty())
      << "The parser did not consume the whole input. Remaining bytes: "
      << ToHumanReadableHexString(binary_encoding);
}

TEST_F(InstructionParserTest, RepeatedLockGroupPrefix) {
  ParseInstructionAndCheckError({0xf0, 0xf2, 0x90}, INVALID_ARGUMENT,
                                "Multiple lock or repeat prefixes were found");
}

TEST_F(InstructionParserTest, ParseNopWithCsSegmentOverride) {
  ParseInstructionAndCheckResult({0x2e, 0x90}, "90+rd", R"proto(
    segment_override: CS_OVERRIDE_OR_BRANCH_NOT_TAKEN
    opcode: 0x90)proto");
}

TEST_F(InstructionParserTest, ParseNopWithSsSegmentOverride) {
  ParseInstructionAndCheckResult({0x36, 0x90}, "90+rd",
                                 "segment_override: SS_OVERRIDE opcode: 0x90");
}

TEST_F(InstructionParserTest, ParseNopWithLockAndSsSegmentOverride) {
  constexpr char kExpectedDecodedInstruction[] = R"proto(
    segment_override: SS_OVERRIDE
    legacy_prefixes { lock_or_rep: LOCK_PREFIX }
    opcode: 0x90)proto";
  ParseInstructionAndCheckResult({0xf0, 0x36, 0x90}, "90+rd",
                                 kExpectedDecodedInstruction);
  ParseInstructionAndCheckResult({0x36, 0xf0, 0x90}, "90+rd",
                                 kExpectedDecodedInstruction);
}

TEST_F(InstructionParserTest, ParseTwoByteOpcode) {
  // INVD
  ParseInstructionAndCheckResult({0x0f, 0x08}, "0F 08", "opcode: 0x0f08");
}

TEST_F(InstructionParserTest, ParseThreeByteOpcode) {
  // CRC32 EAX, AX
  ParseInstructionAndCheckResult({0x0f, 0x38, 0xf1, 0xc0}, "0F 38 F1 /r",
                                 R"proto(
                                   opcode: 0x0f38f1
                                   modrm {
                                     addressing_mode: DIRECT
                                     register_operand: 0
                                     rm_operand: 0
                                   })proto");
}

TEST_F(InstructionParserTest, ParseRexPrefix) {
  // Variants of FCOS with different values of the REX prefix bits. This
  // instruction does not use any operands, so these bits should not have any
  // effect save for rex.w.
  ParseInstructionAndCheckResult(
      {0x48, 0xD9, 0xFF}, "D9 FF",
      "opcode: 0xD9FF legacy_prefixes { rex { w: true } } ");
  ParseInstructionAndCheckResult(
      {0x44, 0xD9, 0xFF}, "D9 FF",
      "opcode: 0xD9FF legacy_prefixes { rex { r: true } } ");
  ParseInstructionAndCheckResult(
      {0x42, 0xD9, 0xFF}, "D9 FF",
      "opcode: 0xD9FF legacy_prefixes { rex { x: true } } ");
  ParseInstructionAndCheckResult(
      {0x41, 0xD9, 0xFF}, "D9 FF",
      "opcode: 0xD9FF legacy_prefixes { rex { b: true } } ");
}

TEST_F(InstructionParserTest, ParseModRmWithBaseOnly) {
  // MOV ECX, DWORD PTR [RBX]
  // Note that there are two ways how to encode this instruction this test
  // executes only the first of them. The next is executed by
  // InstructionParserTest.ParseModRmAndSibWithBaseOnly.
  ParseInstructionAndCheckResult({0x8b, 0x0b}, "8B /r",
                                 R"proto(
                                   opcode: 0x8b
                                   modrm {
                                     addressing_mode: INDIRECT
                                     rm_operand: 3
                                     register_operand: 1
                                   })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWithBaseOnly) {
  // MOV ECX, DWORD PTR [RBX]
  // This is the alternative (three-byte) way to encode this instruction. A
  // two-byte version would use only the ModR/M byte.
  ParseInstructionAndCheckResult({0x8b, 0x0c, 0x23}, "8B /r", R"proto(
    opcode: 0x8b
    modrm { addressing_mode: INDIRECT rm_operand: 4 register_operand: 1 }
    sib { scale: 0 index: 4 base: 3 })proto");
}

TEST_F(InstructionParserTest, ParseModRmWith8BitDisplacement) {
  // MOV ECX, DWORD PTR [RAX + 0x0F]
  // Note that there is an alternative (four-byte) way to encode this
  // instruction. This test executes only the three-byte version. The four-byte
  // version is executed by
  // InstructionParserTest.ParseModRmAndSibWith8BitDisplacement).
  ParseInstructionAndCheckResult({0x8b, 0x48, 0x0f}, "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      rm_operand: 0
      register_operand: 1
      address_displacement: 0xf
    })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWith8BitDisplacement) {
  // MOV ECX, DWORD PTR [RBX + 0x0F]
  ParseInstructionAndCheckResult({0x8b, 0x4c, 0x23, 0x0f}, "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      rm_operand: 4
      register_operand: 1
      address_displacement: 0xf
    }
    sib { scale: 0 index: 4 base: 3 })proto");
}

TEST_F(InstructionParserTest, ParseModRmWith32BitDisplacement) {
  // MOV ECX, DWORD PTR [RAX + 0xFF]
  // Note that since the displacement is a signed integer, 0xFF must be encoded
  // using the 32-bit displacement.
  ParseInstructionAndCheckResult(
      {0x8b, 0x88, 0xff, 0x00, 0x00, 0x00}, "8B /r",
      R"proto(
        opcode: 0x8b
        modrm {
          addressing_mode: INDIRECT_WITH_32_BIT_DISPLACEMENT
          rm_operand: 0
          register_operand: 1
          address_displacement: 0xff
        })proto");
}

TEST_F(InstructionParserTest, ParseModRmWithNegative8BitDisplacement) {
  // MOV ECX, DWORD PTR [RAX - 45]
  ParseInstructionAndCheckResult({0x8b, 0x48, 0xd3}, "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      rm_operand: 0
      register_operand: 1
      address_displacement: -45
    })proto");
}

TEST_F(InstructionParserTest, ParseModRmWithNegative32BitDisplacement) {
  // MOV ECX, DWORD PTR [RAX - 0x1234567890]
  ParseInstructionAndCheckResult(
      {0x8b, 0x88, 0x88, 0xa9, 0xcb, 0xed}, "8B /r",
      R"proto(
        opcode: 0x8b
        modrm {
          addressing_mode: INDIRECT_WITH_32_BIT_DISPLACEMENT
          rm_operand: 0
          register_operand: 1
          address_displacement: -0x12345678
        })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSib) {
  // MOV ECX, DWORD PTR [RBX + 2*RDX]
  ParseInstructionAndCheckResult({0x8b, 0x0c, 0x53}, "8B /r", R"proto(
    opcode: 0x8b
    modrm { addressing_mode: INDIRECT rm_operand: 4 register_operand: 1 }
    sib { scale: 1 base: 3 index: 2 })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWithIndexAnd8BitDisplacement) {
  // MOV ECX, DWORD PTR [RBX + 2*RDX + 4]
  ParseInstructionAndCheckResult({0x8b, 0x4c, 0x53, 0x04}, "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      rm_operand: 4
      register_operand: 1
      address_displacement: 4
    }
    sib { scale: 1 base: 3 index: 2 })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWith32BitDisplacement) {
  // MOV ECX, DWORD PTR [RBX + 2*RDX + 1234]
  ParseInstructionAndCheckResult({0x8b, 0x8c, 0x53, 0xd2, 0x04, 0x00, 0x00},
                                 "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT_WITH_32_BIT_DISPLACEMENT
      rm_operand: 4
      register_operand: 1
      address_displacement: 1234
    }
    sib { scale: 1 base: 3 index: 2 })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWithNoBaseAnd32BitDisplacement) {
  // MOV ECX, DWORD PTR [2*RDX + 12345]
  ParseInstructionAndCheckResult({0x8b, 0x0c, 0x55, 0x39, 0x30, 0x00, 0x00},
                                 "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT
      rm_operand: 4
      register_operand: 1
      address_displacement: 12345
    }
    sib { scale: 1 base: 5 index: 2 })proto");
}

TEST_F(InstructionParserTest, ParseModRmAndSibWith32BitDisplacementOnly) {
  // MOV ECX, DWORD PTR [12345]
  ParseInstructionAndCheckResult({0x8B, 0x0C, 0x25, 0x39, 0x30, 0x00, 0x0},
                                 "8B /r", R"proto(
    opcode: 0x8b
    modrm {
      addressing_mode: INDIRECT
      rm_operand: 4
      register_operand: 1
      address_displacement: 12345
    }
    sib { scale: 0 base: 5 index: 4 })proto");
}

TEST_F(InstructionParserTest, ParseThreeByteVexPrefixWithNonDefaultMapSelect) {
  // ANDN RAX, RBX, RCX
  ParseInstructionAndCheckResult({0xc4, 0xe2, 0xe0, 0xf2, 0xc1},
                                 "VEX.NDS.LZ. 0F38.W1 F2 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F38
                                     inverted_register_operand: 12
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     w: true
                                   }
                                   opcode: 0x0f38f2
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 1
                                     register_operand: 0
                                   })proto");
}

TEST_F(InstructionParserTest, ParseTwoByteVexPrefix) {
  // VADDPD xmm2, xmm3, xmm4
  ParseInstructionAndCheckResult({0xc5, 0xe1, 0x58, 0xd4},
                                 "VEX.NDS.128.66.0F.WIG 58 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F
                                     inverted_register_operand: 12
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     mandatory_prefix:
                                         MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                                   }
                                   opcode: 0x0f58
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 4
                                     register_operand: 2
                                   })proto");
}

TEST_F(InstructionParserTest, ParseTwoByteVexPrefixWithSegmentOverride) {
  // VADDPD xmm2, xmm3, XMMWORD PTR fs:[rbx]
  ParseInstructionAndCheckResult(
      {0x64, 0xc5, 0xe1, 0x58, 0x13}, "VEX.NDS.128.66.0F.WIG 58 /r",
      R"proto(segment_override: FS_OVERRIDE
              vex_prefix {
                map_select: MAP_SELECT_0F
                inverted_register_operand: 12
                not_b: true
                not_r: true
                not_x: true
                mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
              }
              opcode: 0x0f58
              modrm {
                addressing_mode: INDIRECT
                rm_operand: 3
                register_operand: 2
              })proto");
}

TEST_F(InstructionParserTest, ParseTwoByteVexPrefixWithAddressSizeOverride) {
  // VADDPD xmm2, xmm3, XMMWORD PTR [ebx]
  ParseInstructionAndCheckResult(
      {0x67, 0xc5, 0xe1, 0x58, 0x13}, "VEX.NDS.128.66.0F.WIG 58 /r",
      R"proto(address_size_override: ADDRESS_SIZE_OVERRIDE
              vex_prefix {
                map_select: MAP_SELECT_0F
                inverted_register_operand: 12
                not_b: true
                not_r: true
                not_x: true
                mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
              }
              opcode: 0x0f58
              modrm {
                addressing_mode: INDIRECT
                rm_operand: 3
                register_operand: 2
              })proto");
}

TEST_F(InstructionParserTest,
       ParseTwoByteVexPrefixWithAddressSizeAndSegmentOverride) {
  // VADDPD xmm2, xmm3, XMMWORD PTR [ebx]
  ParseInstructionAndCheckResult(
      {0x67, 0x64, 0xc5, 0xe1, 0x58, 0x13}, "VEX.NDS.128.66.0F.WIG 58 /r",
      R"proto(address_size_override: ADDRESS_SIZE_OVERRIDE
              segment_override: FS_OVERRIDE
              vex_prefix {
                map_select: MAP_SELECT_0F
                inverted_register_operand: 12
                not_b: true
                not_r: true
                not_x: true
                mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
              }
              opcode: 0x0f58
              modrm {
                addressing_mode: INDIRECT
                rm_operand: 3
                register_operand: 2
              })proto");
}

TEST_F(InstructionParserTest, ParseTwoByteVexPrefixWithExtendedRegisters) {
  // VADDPD xmm12, xmm13, xmm4
  // The VEX prefix already covers the extended bit for the first operand, and
  // encodes the second operand in full. We can have a two-byte prefix here as
  // long as the third operand is xmm0-xmm7.
  ParseInstructionAndCheckResult({0xc5, 0x11, 0x58, 0xe4},
                                 "VEX.NDS.128.66.0F.WIG 58 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F
                                     inverted_register_operand: 2
                                     not_b: true
                                     not_x: true
                                     mandatory_prefix:
                                         MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                                   }
                                   opcode: 0x0f58
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 4
                                     register_operand: 4
                                   })proto");
}

TEST_F(InstructionParserTest, ParseVaddpdWithExtendedRegisters) {
  // VADDPD xmm12, xmm13, xmm14
  ParseInstructionAndCheckResult({0xc4, 0x41, 0x11, 0x58, 0xe6},
                                 "VEX.NDS.128.66.0F.WIG 58 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F
                                     inverted_register_operand: 2
                                     not_x: true
                                     mandatory_prefix:
                                         MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                                   }
                                   opcode: 0x0f58
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 6
                                     register_operand: 4
                                   })proto");
}

TEST_F(InstructionParserTest, ParseVaddpdWith256Registers) {
  // VADDPD YMM1, YMM5, YMM12
  ParseInstructionAndCheckResult({0xc4, 0xc1, 0x55, 0x58, 0xcc},
                                 "VEX.NDS.256.66.0F.WIG 58 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F
                                     inverted_register_operand: 10
                                     not_r: true
                                     not_x: true
                                     use_256_bit_vector_length: true
                                     mandatory_prefix:
                                         MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                                   }
                                   opcode: 0x0f58
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 4
                                     register_operand: 1
                                   })proto");
}

TEST_F(InstructionParserTest, ParsePextWith64BitValues) {
  // PEXT RAX, RBX, RCX
  ParseInstructionAndCheckResult({0xc4, 0xe2, 0xe2, 0xf5, 0xc1},
                                 "VEX.NDS.LZ.F3.0F38.W1 F5 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F38
                                     inverted_register_operand: 12
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     w: true
                                     mandatory_prefix: MANDATORY_PREFIX_REPE
                                   }
                                   opcode: 0x0f38f5
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 1
                                     register_operand: 0
                                   })proto");
}

TEST_F(InstructionParserTest, ParsePextWith32BitValues) {
  // PEXT EAX, EDX, ESI
  ParseInstructionAndCheckResult({0xc4, 0xe2, 0x6a, 0xf5, 0xc6},
                                 "VEX.NDS.LZ.F3.0F38.W0 F5 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F38
                                     inverted_register_operand: 13
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     mandatory_prefix: MANDATORY_PREFIX_REPE
                                   }
                                   opcode: 0x0f38f5
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 6
                                     register_operand: 0
                                   })proto");
}

TEST_F(InstructionParserTest, ParseShrx) {
  // SHRX RAX, RDX, R14
  ParseInstructionAndCheckResult({0xC4, 0xE2, 0x8B, 0xF7, 0xC2},
                                 "VEX.NDS.LZ.F2.0F38.W1 F7 /r",
                                 R"proto(
                                   vex_prefix {
                                     map_select: MAP_SELECT_0F38
                                     inverted_register_operand: 1
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     w: true
                                     mandatory_prefix: MANDATORY_PREFIX_REPNE
                                   }
                                   opcode: 0x0f38f7
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 2
                                     register_operand: 0
                                   })proto");
}

TEST_F(InstructionParserTest, ParseImmediateValues) {
  // ADC 0xab [to AL]
  ParseInstructionAndCheckResult({0x14, 0xab}, "14 ib",
                                 "opcode: 0x14 immediate_value: '\\xab'");
  // ADC 0xabcd [to AX]
  ParseInstructionAndCheckResult({0x66, 0x15, 0xab, 0xcd}, "66 15 iw", R"proto(
    legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
    opcode: 0x15
    immediate_value: '\xab\xcd')proto");
  // ADC 0xabcdef01 [to EAX]
  ParseInstructionAndCheckResult({0x15, 0xab, 0xcd, 0xef, 0x01}, "15 id",
                                 R"proto(
                                   opcode: 0x15
                                   immediate_value: '\xab\xcd\xef\x01')proto");
}

TEST_F(InstructionParserTest, ParseImmediateValuesWithVexPrefix) {
  // VBLENDPD xmm1, xmm2, xmm3, 4
  ParseInstructionAndCheckResult({0xc4, 0xe3, 0x69, 0x0d, 0xcb, 0x04},
                                 "VEX.NDS.128.66.0F3A.WIG 0D /r ib", R"proto(
    vex_prefix {
      mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
      inverted_register_operand: 13
      not_b: true
      not_r: true
      not_x: true
      map_select: MAP_SELECT_0F3A
    }
    opcode: 0x0f3a0d
    modrm { addressing_mode: DIRECT rm_operand: 3 register_operand: 1 }
    immediate_value: '\x04')proto");
}

TEST_F(InstructionParserTest, ParseMultipleImmediateValues) {
  // ENTER 0xabcd, 0xef
  ParseInstructionAndCheckResult({0xc8, 0xab, 0xcd, 0xef}, "C8 iw ib", R"proto(
    opcode: 0xc8
    immediate_value: '\xab\xcd'
    immediate_value: '\xef')proto");
}

TEST_F(InstructionParserTest, MissingOrIncompleteImmediateValue) {
  // ADC imm8 [the immediate value is missing]
  ParseInstructionAndCheckError({0x14}, INVALID_ARGUMENT,
                                "The immediate value is missing or incomplete");
  // ADC imm16 [the second byte of the immediate value is missing]
  ParseInstructionAndCheckError({0x66, 0x15, 0xab}, INVALID_ARGUMENT,
                                "The immediate value is missing or incomplete");
}

TEST_F(InstructionParserTest, ParseVexSuffix) {
  // VBLENDVPD xmm1, xmm2, xmm3, xmm4
  ParseInstructionAndCheckResult({0xc4, 0xe3, 0x69, 0x4b, 0xcb, 0x40},
                                 "VEX.NDS.128.66.0F3A.W0 4B /r /is4",
                                 R"proto(
                                   vex_prefix {
                                     mandatory_prefix:
                                         MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                                     inverted_register_operand: 13
                                     not_b: true
                                     not_r: true
                                     not_x: true
                                     map_select: MAP_SELECT_0F3A
                                     vex_suffix_value: 0x40
                                   }
                                   opcode: 0x0f3a4b
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 3
                                     register_operand: 1
                                   })proto");
}

TEST_F(InstructionParserTest, MissingVexSuffix) {
  ParseInstructionAndCheckError({0xc4, 0xe3, 0x69, 0x4b, 0xcb},
                                INVALID_ARGUMENT, "The VEX suffix is missing");
}

TEST_F(InstructionParserTest, ParseEvexPrefix) {
  // VMOVSD XMM1 {k4} {z},XMM2,XMM3
  ParseInstructionAndCheckResult({0x62, 0xf1, 0xef, 0x8c, 0x10, 0xcb},
                                 "EVEX.NDS.LIG.F2.0F.W1 10 /r",
                                 R"proto(
                                   evex_prefix {
                                     mandatory_prefix: MANDATORY_PREFIX_REPNE
                                     w: true
                                     map_select: MAP_SELECT_0F
                                     not_r: 3
                                     not_b: true
                                     not_x: true
                                     inverted_register_operand: 29
                                     opmask_register: 4
                                     z: true
                                   }
                                   opcode: 0x0f10
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 3
                                     register_operand: 1
                                   })proto");
  // VMOVSD XMM1 {k1} {z},XMM29,XMM3
  ParseInstructionAndCheckResult({0x62, 0xf1, 0x97, 0x81, 0x10, 0xcb},
                                 "EVEX.NDS.LIG.F2.0F.W1 10 /r",
                                 R"proto(
                                   evex_prefix {
                                     mandatory_prefix: MANDATORY_PREFIX_REPNE
                                     w: true
                                     map_select: MAP_SELECT_0F
                                     not_r: 3
                                     not_b: true
                                     not_x: true
                                     inverted_register_operand: 2
                                     opmask_register: 1
                                     z: true
                                   }
                                   opcode: 0x0f10
                                   modrm {
                                     addressing_mode: DIRECT
                                     rm_operand: 3
                                     register_operand: 1
                                   })proto");
  // The same instruction as above, but one of the reserved bits in the EVEX
  // prefix is set incorrectly.
  ParseInstructionAndCheckError({0x62, 0xf5, 0xef, 0x89, 0x10, 0xcb},
                                INVALID_ARGUMENT, "");
}

TEST_F(InstructionParserTest, OperandEncodedInOpcode) {
  // movabsq  $0xe998686, %rsi
  ParseInstructionAndCheckResult(
      {0x48, 0xBE, 0x86, 0x86, 0x99, 0x0E, 0x00, 0x00, 0x00, 0x00},
      "REX.W + B8+ rd io", R"proto(
        legacy_prefixes { rex { w: true } }
        opcode: 190
        immediate_value: "\206\206\231\016\000\000\000\000"
      )proto");
  // movabsq  $0xe998686, %rax
  ParseInstructionAndCheckResult(
      {0x48, 0xB8, 0x86, 0x86, 0x99, 0x0E, 0x00, 0x00, 0x00, 0x00},
      "REX.W + B8+ rd io", R"proto(
        legacy_prefixes { rex { w: true } }
        opcode: 184
        immediate_value: "\206\206\231\016\000\000\000\000"
      )proto");
  // This instruciton has opcode length > 1 byte.
  // bswapl  %r12d
  ParseInstructionAndCheckResult({0x41, 0x0F, 0xCC}, "0F C8+rd", R"proto(
    legacy_prefixes { rex { b: true } }
    opcode: 4044
  )proto");
  // Instruction with unknown opcode, even after trimming least significant 3
  // bits.
  ParseInstructionAndCheckError(
      {0x48, 0xCE, 0x86, 0x86, 0x99, 0x0E, 0x00, 0x00, 0x00, 0x00}, NOT_FOUND,
      "");
}

TEST_F(InstructionParserTest, MultipleInstructionsWithSimilarOpcode) {
  // NOP
  ParseInstructionAndCheckResult({0x90}, "NP 90", "opcode: 144");
  // xchg %ecx, %eax
  ParseInstructionAndCheckResult({0x93}, "90+rd", "opcode: 147");
}

// POP opcode and XOP prefix has same binary encoding (0x8f), check that we
// parse it always as the POP instruction.
TEST_F(InstructionParserTest, POPvsXOP) {
  // popq -0x50(%rbp)
  ParseInstructionAndCheckResult({0x8F, 0x45, 0xB0}, "8F /0", R"proto(
    opcode: 143
    modrm {
      addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
      rm_operand: 5
      address_displacement: -80
    }
  )proto");
}

TEST_F(InstructionParserTest, X87FpuInstructions) {
  // fadd %st(2), %st(0)
  ParseInstructionAndCheckResult(
      {0xD8, 0xC2}, "D8 /0",
      R"proto(opcode: 0xD8
              modrm { addressing_mode: DIRECT rm_operand: 2 })proto");
  // fadd %st(0), %st(2)
  ParseInstructionAndCheckResult(
      {0xDC, 0xC2}, "DC /0",
      R"proto(opcode: 0xDC
              modrm { addressing_mode: DIRECT rm_operand: 2 })proto");
  // faddq (%rsi)
  ParseInstructionAndCheckResult(
      {0xDC, 0x06}, "DC /0",
      R"proto(opcode: 0xDC
              modrm { addressing_mode: INDIRECT rm_operand: 6 })proto");
  // fcos
  ParseInstructionAndCheckResult({0xD9, 0xFF}, "D9 FF", "opcode: 0xD9FF");
  // fld %st(1)
  ParseInstructionAndCheckResult(
      {0xD9, 0xC1}, "D9 /0",
      R"proto(opcode: 0xD9
              modrm { addressing_mode: DIRECT rm_operand: 1 })proto");
  // flds 0x7b(%rax)
  constexpr const char* const expected_decoded_instruction =
      R"proto(opcode: 0xD9
              modrm {
                addressing_mode: INDIRECT_WITH_8_BIT_DISPLACEMENT
                address_displacement: 0x7B
              })proto";
  ParseInstructionAndCheckResult({0xD9, 0x40, 0x7B}, "D9 /0",
                                 expected_decoded_instruction);
  // fsubl %(rsi)
  ParseInstructionAndCheckResult({0xD8, 0x26}, "D8 /4",
                                 R"proto(opcode: 0xD8
                                         modrm {
                                           addressing_mode: INDIRECT
                                           register_operand: 4
                                           rm_operand: 6
                                         })proto");
}

// XEND is one of the instructions that do not follow the regular multi-byte
// legacy opcode scheme where the only allowed opcode extension bytes are 0F,
// 0F 38, and 0F 3A. We check that both XEND, and its "general" instruction are
// recognized correctly.
TEST_F(InstructionParserTest, ParseXend) {
  // xend
  ParseInstructionAndCheckResult({0x0F, 0x01, 0xD5}, "NP 0F 01 D5",
                                 R"proto(opcode: 0x0F01D5)proto");
  // invlpg (%rdi)
  ParseInstructionAndCheckResult({0x0F, 0x01, 0x3F}, "0F 01/7",
                                 R"proto(opcode: 0x0F01
                                         modrm {
                                           addressing_mode: INDIRECT
                                           register_operand: 7
                                           rm_operand: 7
                                         })proto");
}

TEST_F(InstructionParserTest, ParseLea64) {
  // LEA RDX, [RIP + 0xa3e1e0c]
  ParseInstructionAndCheckResult({0x48, 0x8D, 0x15, 0x0C, 0x1E, 0x3E, 0x0A},
                                 "REX.W + 8D /r", R"proto(
    legacy_prefixes { rex { w: true } }
    opcode: 0x8d
    modrm {
      addressing_mode: INDIRECT
      register_operand: 2
      rm_operand: 5
      address_displacement: 0xa3e1e0c
    }
  )proto");

  // LEA RDI, [RIP + 0xe7769c]
  // The instruction has an operand size override prefix in addition to the
  // REX.W prefix.
  ParseInstructionAndCheckResult(
      {0x66, 0x48, 0x8d, 0x3d, 0x9c, 0x76, 0xe7, 0x0}, "REX.W + 8D /r", R"proto(
        legacy_prefixes {
          rex { w: true }
          operand_size_override: OPERAND_SIZE_OVERRIDE
        }
        opcode: 0x8d
        modrm {
          addressing_mode: INDIRECT
          register_operand: 7
          rm_operand: 5
          address_displacement: 0xe7769c
        })proto");
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
