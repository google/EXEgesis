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

#include "exegesis/x86/cleanup_instruction_set_encoding.h"

#include "absl/status/status.h"
#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "exegesis/testing/test_util.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::StatusIs;
using ::google::protobuf::TextFormat;

TEST(AddMissingModRmAndImmediateSpecificationTest, Vmovd) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVD'
        operands { name: 'xmm1' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: 'VEX.128.66.0F.W0 6E'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVD'
        operands { name: 'xmm1' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: 'VEX.128.66.0F.W0 6E /r'
    })pb";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingModRmAndImmediateSpecificationTest, Kshiftlb) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'KSHIFTLB'
        operands { name: 'k1' }
        operands { name: 'k2' }
        operands { name: 'imm8' }
      }
      raw_encoding_specification: 'VEX.L0.66.0F3A.W0 32 /r'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'KSHIFTLB'
        operands { name: 'k1' }
        operands { name: 'k2' }
        operands { name: 'imm8' }
      }
      raw_encoding_specification: 'VEX.L0.66.0F3A.W0 32 /r ib'
    })pb";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingMemoryOffsetEncodingTest, AddOffset) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'AAD'
        operands { name: 'imm8' }
      }
      raw_encoding_specification: 'D5 ib'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'AL' }
        operands { name: 'moffs8' }
      }
      raw_encoding_specification: 'A0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'RAX' }
        operands { name: 'moffs64' }
      }
      legacy_instruction: false
      raw_encoding_specification: 'REX.W + A1'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'AAD'
        operands { name: 'imm8' }
      }
      raw_encoding_specification: 'D5 ib'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'AL' }
        operands { name: 'moffs8' }
      }
      raw_encoding_specification: 'A0 io'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'RAX' }
        operands { name: 'moffs64' }
      }
      legacy_instruction: false
      raw_encoding_specification: 'REX.W + A1 io'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'AL' }
        operands { name: 'moffs8' }
      }
      raw_encoding_specification: '67 A0 id'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'RAX' }
        operands { name: 'moffs64' }
      }
      legacy_instruction: false
      raw_encoding_specification: '67 REX.W + A1 id'
    })pb";
  TestTransform(AddMissingMemoryOffsetEncoding, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingModRmAndImmediateSpecificationTest, NoChange) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'AAD'
        operands { name: 'imm8' }
      }
      raw_encoding_specification: 'D5 ib'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVD'
        operands { name: 'xmm1' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: 'VEX.128.66.0F.W0 6E /r'
    })pb";
  TestTransform(AddMissingModRmAndImmediateSpecification, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(FixAndCleanUpEncodingSpecificationsOfSetInstructionsTest,
     SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'SETA'
        operands { name: 'r/m8' }
      }
      raw_encoding_specification: '0F 97'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SETA'
        operands { name: 'r/m8' }
      }
      raw_encoding_specification: 'REX + 0F 97'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'BYTE PTR [RDI]' }
        operands { name: 'AL' }
      }
      raw_encoding_specification: 'AA'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'SETA'
        operands { name: 'r/m8' }
      }
      raw_encoding_specification: '0F 97 /0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'BYTE PTR [RDI]' }
        operands { name: 'AL' }
      }
      raw_encoding_specification: 'AA'
    })pb";
  TestTransform(FixAndCleanUpEncodingSpecificationsOfSetInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(FixEncodingSpecificationOfPopFsAndGsTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      description: 'Pop top of stack into FS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: '0F A1'
    }
    instructions {
      description: 'Pop top of stack into FS. Increment stack pointer by 16 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F A1'
    }
    instructions {
      description: 'Pop top of stack into GS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: '0F A9'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      description: 'Pop top of stack into FS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: '0F A1'
    }
    instructions {
      description: 'Pop top of stack into FS. Increment stack pointer by 16 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 0F A1'
    }
    instructions {
      description: 'Pop top of stack into GS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: '0F A9'
    }
    instructions {
      description: 'Pop top of stack into FS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: 'REX.W 0F A1'
    }
    instructions {
      description: 'Pop top of stack into GS. Increment stack pointer by 64 bits.'
      vendor_syntax {
        mnemonic: 'POP'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      legacy_instruction: false
      raw_encoding_specification: 'REX.W 0F A9'
    })pb";
  TestTransform(FixEncodingSpecificationOfPopFsAndGs, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixEncodingSpecificationOfPushFsAndGsTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F A0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F A8'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F A0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '0F A8'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 0F A0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'FS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'REX.W 0F A0'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: '66 0F A8'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PUSH'
        operands {
          name: 'GS'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 16
        }
      }
      raw_encoding_specification: 'REX.W 0F A8'
    })pb";
  TestTransform(FixEncodingSpecificationOfPushFsAndGs, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixEncodingSpecificationOfXBeginTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'XBEGIN'
        operands { name: 'rel16' }
      }
      raw_encoding_specification: 'C7 F8'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'XBEGIN'
        operands { name: 'rel32' }
      }
      raw_encoding_specification: 'C7 F8'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'XBEGIN'
        operands { name: 'rel16' }
      }
      raw_encoding_specification: '66 C7 F8 cw'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'XBEGIN'
        operands { name: 'rel32' }
      }
      raw_encoding_specification: 'C7 F8 cd'
    })pb";
  TestTransform(FixEncodingSpecificationOfXBegin, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixEncodingSpecificationsTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PCMPISTRI'
        operands { name: 'xmm1' }
        operands { name: 'm128' }
        operands { name: 'imm8' }
      }
      raw_encoding_specification: '66 0F 3A 63 /r imm8'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PMOVSXBW'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0f 38 20 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: "GF2P8AFFINEQB"
        operands { name: "xmm1" }
        operands { name: "xmm2/m128" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "66 0F3A CE /r /ib"
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.W0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PCMPISTRI'
        operands { name: 'xmm1' }
        operands { name: 'm128' }
        operands { name: 'imm8' }
      }
      raw_encoding_specification: '66 0F 3A 63 /r ib'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PMOVSXBW'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0F 38 20 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: "GF2P8AFFINEQB"
        operands { name: "xmm1" }
        operands { name: "xmm2/m128" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "66 0F3A CE /r ib"
    })pb";
  TestTransform(FixEncodingSpecifications, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(DropModRmModDetailsFromEncodingSpecifications, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: "GF2P8AFFINEQB"
        operands { name: "xmm1" }
        operands { name: "xmm2/m128" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "66 0F3A CE /r ib"
    }
    instructions {
      vendor_syntax {
        mnemonic: "RDSSPD"
        operands { name: "r32" }
      }
      feature_name: "CET_SS"
      raw_encoding_specification: "F3 0F 1E /1 (mod=11)"
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: "GF2P8AFFINEQB"
        operands { name: "xmm1" }
        operands { name: "xmm2/m128" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "66 0F3A CE /r ib"
    }
    instructions {
      vendor_syntax {
        mnemonic: "RDSSPD"
        operands { name: "r32" }
      }
      feature_name: "CET_SS"
      raw_encoding_specification: "F3 0F 1E /1"
    })pb";
  TestTransform(DropModRmModDetailsFromEncodingSpecifications,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(FixRexPrefixSpecificationTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: "MOVSX"
        operands { name: "r64" }
        operands { name: "m8" }
      }
      raw_encoding_specification: "REX + 0F BE /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "r8" }
      }
      raw_encoding_specification: "REX + 10 /r"
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: "MOVSX"
        operands { name: "r64" }
        operands { name: "m8" }
      }
      raw_encoding_specification: "REX.W + 0F BE /r"
    }
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "m8" }
        operands { name: "r8" }
      }
      raw_encoding_specification: "REX + 10 /r"
    })pb";
  TestTransform(FixRexPrefixSpecification, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(ParseEncodingSpecificationsTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.W0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PMOVSXBW'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0F 38 20 /r'
    })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.W0 BA /r'
      x86_encoding_specification {
        opcode: 997562
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PMOVSXBW'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0F 38 20 /r'
      x86_encoding_specification {
        opcode: 997408
        modrm_usage: FULL_MODRM
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_REQUIRED }
      }
    })pb";
  TestTransform(ParseEncodingSpecifications, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(ParseEncodingSpecificationsTest, ParseErrors) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMSUB231PS'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'm128' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.0 BA /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'PMOVSXBW'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0F 38 20 /r'
    })pb";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  EXPECT_THAT(ParseEncodingSpecifications(&instruction_set),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ConvertEncodingSpecificationOfX87FpuWithDirectAddressingTest,
     SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "D8 F0+i" }
    instructions { raw_encoding_specification: "DC C0+i" }
    instructions { raw_encoding_specification: "D8 /6" })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "D8 /6" }
    instructions { raw_encoding_specification: "DC /0" }
    instructions { raw_encoding_specification: "D8 /6" })pb";
  TestTransform(ConvertEncodingSpecificationOfX87FpuWithDirectAddressing,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddRexWPrefixedVersionOfStrTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "0F 00 /1" }
    instructions { raw_encoding_specification: "66 0F 38 20 /r" }
    instructions { raw_encoding_specification: "D8 /6" })pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "0F 00 /1" }
    instructions { raw_encoding_specification: "66 0F 38 20 /r" }
    instructions { raw_encoding_specification: "D8 /6" }
    instructions { raw_encoding_specification: "REX.W 0F 00 /1" })pb";
  TestTransform(AddRexWPrefixedVersionOfStr, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(NormalizeEncodingSpecificationLigFlagTest, SomeInstructions) {
  constexpr char kInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "VEX.LIG.F2.0F.WIG 58 /r" }
    instructions { raw_encoding_specification: "EVEX.LLIG.F2.0F38.W0 9B /r" }
    instructions { raw_encoding_specification: "0F 00 /1" }
  )pb";
  constexpr char kExpectedInstructionSetProto[] = R"pb(
    instructions { raw_encoding_specification: "VEX.LIG.F2.0F.WIG 58 /r" }
    instructions { raw_encoding_specification: "EVEX.LIG.F2.0F38.W0 9B /r" }
    instructions { raw_encoding_specification: "0F 00 /1" }
  )pb";
  TestTransform(NormalizeEncodingSpecificationLigFlag, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
