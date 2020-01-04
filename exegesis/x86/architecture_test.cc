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

// Contains tests for validating the x86-64 instruction set.

#include "exegesis/x86/architecture.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace x86 {
namespace {

using ::testing::Contains;
using ::testing::UnorderedElementsAreArray;

constexpr char kArchitectureProto[] = R"proto(
  instruction_set {
    instructions {
      llvm_mnemonic: "BLSMSK64rr"
      vendor_syntax {
        mnemonic: "BLSMSK"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_READ
        }
      }
      feature_name: "BMI1"
      raw_encoding_specification: "VEX.NDD.LZ.0F38.W1 F3 /2"
      x86_encoding_specification {
        opcode: 0x0f38f3
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_DESTINATION_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ONE
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "MOV" }
      raw_encoding_specification: "B8+ rd id"
      x86_encoding_specification {
        opcode: 184
        operand_in_opcode: GENERAL_PURPOSE_REGISTER_IN_OPCODE
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 4
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
      vendor_syntax {
        mnemonic: "ADD"
        operands { name: "m8" }
        operands { name: "r8" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'MR'
      raw_encoding_specification: '00 /r'
      x86_encoding_specification {
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AL" }
        operands { name: "imm8" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'I'
      raw_encoding_specification: '14 ib'
      x86_encoding_specification {
        opcode: 0x14
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
        immediate_value_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "RAX" }
        operands { name: "imm32" }
      }
      available_in_64_bit: true
      legacy_instruction: false
      encoding_scheme: 'I'
      raw_encoding_specification: 'REX.W + 15 id'
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_REQUIRED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AX" }
        operands { name: "imm16" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'I'
      raw_encoding_specification: '66 15 iw'
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
        immediate_value_bytes: 2
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "EAX" }
        operands { name: "imm32" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'I'
      raw_encoding_specification: '15 id'
      x86_encoding_specification {
        opcode: 0x15
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        immediate_value_bytes: 4
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "JB"
        operands { name: "rel8" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'D'
      raw_encoding_specification: '7F cb'
      x86_encoding_specification {
        opcode: 0x7F
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
        code_offset_bytes: 1
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "NOT"
        operands { name: "m8" }
      }
      available_in_64_bit: true
      legacy_instruction: false
      encoding_scheme: 'M'
      raw_encoding_specification: 'REX + F6 /2'
      x86_encoding_specification {
        opcode: 0xF6
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 2
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VERR"
        operands { name: "m16" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'M'
      raw_encoding_specification: '0F 00 /4'
      x86_encoding_specification {
        opcode: 0x0F00
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 4
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VUNPCKHPD"
        operands { name: "ymm1" }
        operands { name: "ymm2" }
        operands { name: "m256" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'RVM'
      raw_encoding_specification: 'VEX.NDS.256.66.0F.WIG 15 /r'
      x86_encoding_specification {
        opcode: 0x0F15
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
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          name: "xmm1"
          tags { name: "k1" }
          tags { name: "z" }
        }
        operands { name: "xmm2" }
        operands { name: "m128" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'FV-RVM'
      raw_encoding_specification: 'EVEX.NDS.128.66.0F.W1 58 /r'
      x86_encoding_specification {
        opcode: 0x0F58
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ONE
          evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VPADDD"
        operands {
          name: "xmm1"
          tags { name: "k1" }
          tags { name: "z" }
        }
        operands { name: "xmm2" }
        operands { name: "m128" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'FV'
      raw_encoding_specification: 'EVEX.NDS.128.66.0F.W0 FE /r'
      x86_encoding_specification {
        opcode: 0x0FFE
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          evex_b_interpretations: EVEX_B_ENABLES_32_BIT_BROADCAST
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VPADDD"
        operands { name: "ymm1" }
        operands { name: "ymm2" }
        operands { name: "m256" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'RVM'
      raw_encoding_specification: 'VEX.NDS.256.66.0F.WIG FE /r'
      x86_encoding_specification {
        opcode: 0x0FFE
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
      vendor_syntax {
        mnemonic: "VPADDD"
        operands { name: "xmm1" }
        operands { name: "xmm2" }
        operands { name: "m128" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: 'RVM'
      raw_encoding_specification: 'VEX.NDS.128.66.0F.WIG FE /r'
      x86_encoding_specification {
        opcode: 0x0FFE
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
      vendor_syntax { mnemonic: "XEND" }
      feature_name: "RTM"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "A"
      raw_encoding_specification: "NP 0F 01 D5"
      x86_encoding_specification {
        opcode: 983509
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "KSHIFTLD"
        operands { name: "k1" }
        operands { name: "k2" }
        operands { name: "imm8" }
      }
      feature_name: "AVX512BW"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RRI"
      raw_encoding_specification: "VEX.L0.66.0F3A.W0 33 /r ib"
      x86_encoding_specification {
        opcode: 997939
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F3A
          vex_w_usage: VEX_W_IS_ZERO
        }
        immediate_value_bytes: 1
      }
    }
  })proto";

class X86ArchitectureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    architecture_proto_ = std::make_shared<ArchitectureProto>();
    ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(
        kArchitectureProto, architecture_proto_.get()));
    architecture_ = absl::make_unique<X86Architecture>(architecture_proto_);
  }

  std::shared_ptr<ArchitectureProto> architecture_proto_;
  std::unique_ptr<X86Architecture> architecture_;
};

TEST_F(X86ArchitectureTest, InstructionSetIsNotEmpty) {
  EXPECT_GT(architecture_->num_instructions(), 0);
}

TEST_F(X86ArchitectureTest, GetOpcodesReturnsAllOpcodes) {
  const OpcodeSet opcodes = architecture_->GetOpcodes();
  for (X86Architecture::InstructionIndex i(0);
       i < architecture_->num_instructions(); ++i) {
    const EncodingSpecification& encoding_specification =
        architecture_->encoding_specification(i);
    const Opcode opcode = Opcode(encoding_specification.opcode());
    EXPECT_TRUE(gtl::ContainsKey(opcodes, opcode))
        << "Opcode was missing: " << opcode;
  }
}

TEST_F(X86ArchitectureTest, IsLegacyOpcodePrefix) {
  // Check that only the proper prefixes are added for XEND (0F 01 D5).
  EXPECT_TRUE(architecture_->IsLegacyOpcodePrefix(Opcode(0x0F)));
  EXPECT_TRUE(architecture_->IsLegacyOpcodePrefix(Opcode(0x0F01)));
  EXPECT_FALSE(architecture_->IsLegacyOpcodePrefix(Opcode(0x0F01D5)));

  // Check that the prefix of a VEX-encoded instruction is not added.
  EXPECT_FALSE(architecture_->IsLegacyOpcodePrefix(Opcode(0x0F3A)));
}

void CheckInstructionIndex(const X86Architecture& architecture,
                           const DecodedInstruction& instruction,
                           const char* expected_raw_encoding_specification,
                           bool check_modrm) {
  const X86Architecture::InstructionIndex instruction_index =
      architecture.GetInstructionIndex(instruction, check_modrm);
  if (expected_raw_encoding_specification == nullptr) {
    EXPECT_EQ(instruction_index, X86Architecture::kInvalidInstruction);
  } else {
    ASSERT_NE(instruction_index, X86Architecture::kInvalidInstruction);
    const InstructionProto& instruction_proto =
        architecture.instruction(instruction_index);
    EXPECT_EQ(instruction_proto.raw_encoding_specification(),
              expected_raw_encoding_specification);
  }
}

TEST_F(X86ArchitectureTest, GetInstructionIndex) {
  constexpr struct {
    const char* encoded_instruction_proto;
    const char* expected_raw_encoding_specification_with_modrm;
    const char* expected_raw_encoding_specification_without_modrm;
  } kTestCases[] = {
      {"opcode: 0x14", "14 ib", "14 ib"},
      {"opcode: 0x15", "15 id", "15 id"},
      // movl $0x12345678, %ecx
      // To check we can resolve opcodes that encode an operand.
      {R"proto(opcode: 0xB9 immediate_value: "xV4\022")proto", "B8+ rd id",
       "B8+ rd id"},
      {"opcode: 148", "90+rd", "90+rd"},
      {"opcode: 22", nullptr, nullptr},
      {R"proto(legacy_prefixes { operand_size_override: OPERAND_SIZE_OVERRIDE }
               opcode: 0x15)proto",
       "66 15 iw", "66 15 iw"},
      {R"proto(
         legacy_prefixes { rex { w: true } }
         opcode: 0x15)proto",
       "REX.W + 15 id", "REX.W + 15 id"},
      {R"proto(vex_prefix {
                 not_b: true
                 not_r: true
                 not_x: true
                 w: true
                 map_select: MAP_SELECT_0F38
               }
               opcode: 0x0f38f3
               modrm { register_operand: 2 addressing_mode: DIRECT }
       )proto",
       "VEX.NDD.LZ.0F38.W1 F3 /2", "VEX.NDD.LZ.0F38.W1 F3 /2"},
      {R"proto(vex_prefix {
                 not_b: true
                 not_r: true
                 not_x: true
                 w: true
                 map_select: MAP_SELECT_0F38
               }
               opcode: 0x0f38f3
               modrm { register_operand: 7 })proto",
       nullptr, "VEX.NDD.LZ.0F38.W1 F3 /2"},
      // The opcode 0x14 always uses 8-bit values. Prefixes affecting the size
      // of the instruction are ignored.
      {R"proto(legacy_prefixes { rex { w: true } }
               opcode: 0x14)proto",
       "14 ib", "14 ib"},
      {R"proto(vex_prefix {
                 mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                 map_select: MAP_SELECT_0F
                 use_256_bit_vector_length: true
               }
               opcode: 0x0ffe)proto",
       "VEX.NDS.256.66.0F.WIG FE /r", "VEX.NDS.256.66.0F.WIG FE /r"},
      {R"proto(vex_prefix {
                 mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                 map_select: MAP_SELECT_0F
                 use_256_bit_vector_length: true
                 w: true
               }
               opcode: 0x0ffe)proto",
       "VEX.NDS.256.66.0F.WIG FE /r", "VEX.NDS.256.66.0F.WIG FE /r"},
      {R"proto(vex_prefix {
                 mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                 map_select: MAP_SELECT_0F
               }
               opcode: 0x0ffe)proto",
       "VEX.NDS.128.66.0F.WIG FE /r", "VEX.NDS.128.66.0F.WIG FE /r"},
      {R"proto(evex_prefix {
                 mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                 map_select: MAP_SELECT_0F
                 w: true
                 vector_length_or_rounding: 0
               }
               opcode: 0x0f58)proto",
       "EVEX.NDS.128.66.0F.W1 58 /r", "EVEX.NDS.128.66.0F.W1 58 /r"}};
  for (const auto test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat("test_case.encoded_instruction_proto:\n",
                              test_case.encoded_instruction_proto));
    const auto instruction = ParseProtoFromStringOrDie<DecodedInstruction>(
        test_case.encoded_instruction_proto);
    CheckInstructionIndex(
        *architecture_, instruction,
        test_case.expected_raw_encoding_specification_with_modrm, true);
    CheckInstructionIndex(
        *architecture_, instruction,
        test_case.expected_raw_encoding_specification_without_modrm, false);
  }
}

TEST_F(X86ArchitectureTest, GetInstructionIndices) {
  const struct {
    const std::string encoded_instruction_proto;
    std::vector<std::string> expected_raw_encoding_specification;
  } kTestCases[] = {{"opcode: 144", {"NP 90", "90+rd"}},
                    {"opcode: 148", {"90+rd"}},
                    {"opcode: 22", {}}};
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat("test_case.encoded_instruction_proto:\n",
                              test_case.encoded_instruction_proto));
    const auto instruction = ParseProtoFromStringOrDie<DecodedInstruction>(
        test_case.encoded_instruction_proto);
    const std::vector<X86Architecture::InstructionIndex> instruction_indices =
        architecture_->GetInstructionIndices(instruction, true);
    std::vector<std::string> actual_raw_encoding_specification;
    for (const auto& instruction_index : instruction_indices) {
      const InstructionProto& instruction_proto =
          architecture_->instruction(instruction_index);
      actual_raw_encoding_specification.push_back(
          instruction_proto.raw_encoding_specification());
    }
    EXPECT_THAT(actual_raw_encoding_specification,
                UnorderedElementsAreArray(
                    test_case.expected_raw_encoding_specification));
  }
}

// Checks that for each instruction:
// 1. GetInstructionIndicesByOpcode() returns its own index when searching for
//    it,
// 2. all instructions returned by GetInstructionIndicesByOpcode() have the same
//    opcode.
TEST_F(X86ArchitectureTest, GetInstructionIndicesByOpcode) {
  for (X86Architecture::InstructionIndex instruction_index(0);
       instruction_index < architecture_->num_instructions();
       ++instruction_index) {
    const InstructionProto& instruction =
        architecture_->instruction(instruction_index);
    EXPECT_TRUE(instruction.has_x86_encoding_specification());
    const Opcode opcode(instruction.x86_encoding_specification().opcode());
    const std::vector<X86Architecture::InstructionIndex> indices =
        architecture_->GetInstructionIndicesByOpcode(opcode);
    EXPECT_THAT(indices, Contains(instruction_index));
    for (const X86Architecture::InstructionIndex other_index : indices) {
      const InstructionProto& other_instruction =
          architecture_->instruction(other_index);
      const Opcode other_opcode(
          other_instruction.x86_encoding_specification().opcode());
      EXPECT_EQ(other_opcode, opcode);
    }
  }
}

// Converts the list of ModR/M usages from 'container' to a human-readable
// string.
template <typename Container>
std::string ModRmUsagesToString(const Container& container) {
  return absl::StrJoin(
      container, ", ",
      [](std::string* out, const EncodingSpecification::ModRmUsage usage) {
        absl::StrAppend(out, EncodingSpecification::ModRmUsage_Name(usage));
      });
}

// Verifies that the ModR/M byte is used consistently, i.e. instructions using
// the same encoding and having the same opcode always have the same ModR/M
// interpretation.
// Note that legacy instructions and (E)VEX instructions with the same opcode
// generally do not have the same function (and as a consequence, we can't
// expect them to be consistent with respect to the use of the ModR/M byte).
TEST_F(X86ArchitectureTest, ModRmUsageIsConsistentAcrossOpcodes) {
  const OpcodeSet opcodes = architecture_->GetOpcodes();

  for (const Opcode opcode : opcodes) {
    SCOPED_TRACE(absl::StrCat("Opcode: ", opcode.ToString()));
    const std::vector<X86Architecture::InstructionIndex> instruction_indices =
        architecture_->GetInstructionIndicesByOpcode(opcode);
    std::set<EncodingSpecification::ModRmUsage> legacy_modrm_usages;
    std::set<EncodingSpecification::ModRmUsage> vex_modrm_usages;
    for (const X86Architecture::InstructionIndex index : instruction_indices) {
      const EncodingSpecification& encoding =
          architecture_->encoding_specification(index);
      if (encoding.has_vex_prefix()) {
        vex_modrm_usages.insert(encoding.modrm_usage());
      } else {
        legacy_modrm_usages.insert(encoding.modrm_usage());
      }
    }
    EXPECT_LE(legacy_modrm_usages.size(), 1)
        << "Inconsistent ModR/M usage for legacy instructions, opcode: "
        << opcode << "\nUsages: " << ModRmUsagesToString(legacy_modrm_usages);
    EXPECT_LE(vex_modrm_usages.size(), 1)
        << "Inconsisten ModR/M usage for (E)VEX instructions, opcode: "
        << opcode << "\nUsages: " << ModRmUsagesToString(vex_modrm_usages);
  }
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
