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

#include "exegesis/x86/cleanup_instruction_set_merge_instructions.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

// Tests that the transform correctly merges STOS BYTE PTR and STOSB
// instructions, but it does not merge them with STOSW which is a different
// instruction.
TEST(MergeVendorSyntaxTest, MergeStosAndStosb) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "STOS"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_BY_RDI
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
          name: "WORD PTR [RDI]"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
          name: "AX"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_16_BIT
        }
      }
      syntax {
        mnemonic: "stosw"
        operands { name: "word ptr es:[rdi]" }
        operands { name: "ax" }
      }
      att_syntax {
        mnemonic: "stosw"
        operands { name: "%ax" }
        operands { name: "%es:(%rdi)" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "NA"
      raw_encoding_specification: "66 AB"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 171
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "STOS"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_BY_RDI
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
          name: "BYTE PTR [RDI]"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
          name: "AL"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_8_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "NA"
      raw_encoding_specification: "AA"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 170
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    }
    instructions {
      vendor_syntax { mnemonic: "STOSW" }
      syntax { mnemonic: "stosw" }
      att_syntax { mnemonic: "stosw" }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "NA"
      raw_encoding_specification: "66 AB"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 171
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "STOS"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_BY_RDI
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
          name: "WORD PTR [RDI]"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
          name: "AX"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_16_BIT
        }
      }
      vendor_syntax { mnemonic: "STOSW" }
      syntax {
        mnemonic: "stosw"
        operands { name: "word ptr es:[rdi]" }
        operands { name: "ax" }
      }
      att_syntax {
        mnemonic: "stosw"
        operands { name: "%ax" }
        operands { name: "%es:(%rdi)" }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "NA"
      raw_encoding_specification: "66 AB"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 171
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_REQUIRED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "STOS"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_BY_RDI
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
          name: "BYTE PTR [RDI]"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
          name: "AL"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_8_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "NA"
      raw_encoding_specification: "AA"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 170
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
    })proto";
  TestTransform(MergeVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

// Tests that the transform does not merge two versions of an instruction using
// different addressing modes. From performance point of view, these are two
// different instructions.
TEST(MergeVendorSyntaxTest, DoesNotMergeAcrossAddressingModes) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "03 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 3
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "m32"
          usage: USAGE_READ
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "03 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 3
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "03 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 3
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "m32"
          usage: USAGE_READ
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "03 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 3
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  TestTransform(MergeVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

// Checks that the instruction merges two versions of an instruction where the
// operands are explicit operands, and only their order in the vendor syntax is
// updated.
TEST(MergeVendorSyntaxTest, MergesReorderedOperands) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      description: "Exchange r32 with doubleword from r/m32."
      llvm_mnemonic: "XCHG32rr"
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "MR"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
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
      description: "Exchange doubleword from r/m32 with r32."
      llvm_mnemonic: "XCHG32rr"
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      description: "Exchange r32 with doubleword from r/m32."
      llvm_mnemonic: "XCHG32rr"
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "MR"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  TestTransform(MergeVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveUselessOperandPermutationsTest, RemoveUselessPermutations) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "MR"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "MR"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  TestTransform(RemoveUselessOperandPermutations, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveUselessOperandPermutationsTest, KeepUsefulPermutations) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "m32"
          usage: USAGE_READ_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
      }
      vendor_syntax {
        mnemonic: "XCHG"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_READ_WRITE
          register_class: GENERAL_PURPOSE_REGISTER_32_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
          name: "m32"
          usage: USAGE_READ_WRITE
        }
      }
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "MR"
      raw_encoding_specification: "87 /r"
      protection_mode: -1
      x86_encoding_specification {
        opcode: 135
        modrm_usage: FULL_MODRM
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_NOT_PERMITTED
          operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
        }
      }
    })proto";
  TestTransform(RemoveUselessOperandPermutations, kInstructionSetProto,
                kInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
