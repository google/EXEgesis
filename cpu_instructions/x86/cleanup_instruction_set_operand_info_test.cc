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

#include "cpu_instructions/x86/cleanup_instruction_set_operand_info.h"

#include "src/google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {
namespace {

using ::cpu_instructions::util::Status;
using ::cpu_instructions::util::error::INVALID_ARGUMENT;

TEST(AddOperandInfoTest, AddInfo) {
  constexpr char kInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands { name: 'BYTE PTR [RDI]' } operands { name: 'AL' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' }
         instructions {
           vendor_syntax {
             mnemonic: 'FMUL'
             operands { name: 'ST(0)' } operands { name: 'ST(i)' }}
           feature_name: 'X87'
           binary_encoding: 'D8 C8+i' }
         instructions {
           vendor_syntax {
             mnemonic: 'VMOVD'
             operands { name: 'xmm1' encoding: MODRM_REG_ENCODING }
             operands { name: 'r32' }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W0 6E /r' })";
  constexpr char kExpectedInstructionSetProto[] =
      R"(instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands { name: 'BYTE PTR [RDI]'
                        addressing_mode: INDIRECT_ADDRESSING_BY_RDI
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 8 }
             operands { name: 'AL' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 8 }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' }
         instructions {
           vendor_syntax {
             mnemonic: 'FMUL'
             operands { name: 'ST(0)' addressing_mode: DIRECT_ADDRESSING
                        encoding: IMPLICIT_ENCODING
                        value_size_bits: 80 }
             operands { name: 'ST(i)' addressing_mode: DIRECT_ADDRESSING
                        encoding: OPCODE_ENCODING
                        value_size_bits: 80 }}
           feature_name: 'X87'
           binary_encoding: 'D8 C8+i' }
         instructions {
           vendor_syntax {
             mnemonic: 'VMOVD'
             operands { name: 'xmm1' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_REG_ENCODING
                        value_size_bits: 128 }
             operands { name: 'r32' addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32 }}
           feature_name: 'AVX'
           encoding_scheme: 'RM'
           binary_encoding: 'VEX.128.66.0F.W0 6E /r' })";
  TestTransform(AddOperandInfo, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddOperandInfoTest, DetectsInconsistentEncodings) {
  constexpr const char* const kInstructionSetProtos[] = {
      // The instruction encoding does not use the ModR/M byte, so the operands
      // can't use MODRM_RM_ENCODING.
      R"(instructions {
           vendor_syntax {
             mnemonic: 'STOS'
             operands { name: 'BYTE PTR [RDI]' encoding: MODRM_RM_ENCODING }
             operands { name: 'AL' }}
           encoding_scheme: 'NA'
           binary_encoding: 'AA' })",
      // Only one operand can be encoded in the opcode.
      R"(instructions {
           vendor_syntax {
             mnemonic: 'FMUL'
             operands { name: 'ST(0)' encoding: OPCODE_ENCODING }
             operands { name: 'ST(i)' encoding: OPCODE_ENCODING }}
           feature_name: 'X87'
           binary_encoding: 'D8 C8+i' })"};
  for (const char* const instruction_set_proto : kInstructionSetProtos) {
    InstructionSetProto instruction_set;
    ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(instruction_set_proto,
                                                      &instruction_set));
    const Status transform_status = AddOperandInfo(&instruction_set);
    EXPECT_EQ(transform_status.error_code(), INVALID_ARGUMENT);
  }
}

TEST(AddMissingOperandUsageTest, AddMissingOperandUsage) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: 'STUFF'
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
          }
          operands {
            name: 'imm64'
            encoding: IMMEDIATE_VALUE_ENCODING
          }
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
          }
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
            usage: USAGE_WRITE
          }
          operands {
            name: 'xmm1'
            addressing_mode: DIRECT_ADDRESSING
            encoding: VEX_V_ENCODING
          }
        }
      })";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: 'STUFF'
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
            usage: USAGE_WRITE
          }
          operands {
            name: 'imm64'
            encoding: IMMEDIATE_VALUE_ENCODING
            usage: USAGE_READ
          }
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
            usage: USAGE_READ
          }
          operands {
            name: 'r64'
            addressing_mode: DIRECT_ADDRESSING
            encoding: IMPLICIT_ENCODING
            usage: USAGE_WRITE
          }
          operands {
            name: 'xmm1'
            addressing_mode: DIRECT_ADDRESSING
            encoding: VEX_V_ENCODING
            usage: USAGE_READ
          }
        }
      })";
  TestTransform(AddMissingOperandUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
