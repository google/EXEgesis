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

#include "exegesis/x86/cleanup_instruction_set_fix_operands.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::IsInvalidArgument;
using ::exegesis::util::Status;

TEST(FixOperandsOfCmpsAndMovsTest, Instructions) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'MOVS'
        operands { name: 'm16' encoding: IMPLICIT_ENCODING }
        operands { name: 'm16' }
      }
      raw_encoding_specification: 'A5'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVS'
        operands { name: 'm32' }
        operands { name: 'm32' }
      }
      raw_encoding_specification: 'A5'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CMPS'
        operands { name: 'm8' }
        operands { name: 'm8' encoding: IMPLICIT_ENCODING }
      }
      raw_encoding_specification: 'A6'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CMPS'
        operands { name: 'm64' }
        operands { name: 'm64' }
      }
      raw_encoding_specification: 'REX.W + A7'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'MOVS'
        operands {
          name: 'WORD PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_WRITE
        }
        operands { name: 'WORD PTR [RSI]' usage: USAGE_READ }
      }
      raw_encoding_specification: 'A5'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVS'
        operands { name: 'DWORD PTR [RDI]' usage: USAGE_WRITE }
        operands { name: 'DWORD PTR [RSI]' usage: USAGE_READ }
      }
      raw_encoding_specification: 'A5'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CMPS'
        operands { name: 'BYTE PTR [RSI]' usage: USAGE_READ }
        operands {
          name: 'BYTE PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'A6'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'CMPS'
        operands { name: 'QWORD PTR [RSI]' usage: USAGE_READ }
        operands { name: 'QWORD PTR [RDI]' usage: USAGE_READ }
      }
      raw_encoding_specification: 'REX.W + A7'
    })proto";
  TestTransform(FixOperandsOfCmpsAndMovs, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfInsAndOutsTest, Ins) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
        operands { name: 'DX' }
      }
      raw_encoding_specification: '6C'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm16' }
        operands { name: 'DX' }
      }
      raw_encoding_specification: '6D'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'BYTE PTR [RDI]' usage: USAGE_WRITE }
        operands { name: 'DX' usage: USAGE_READ }
      }
      raw_encoding_specification: '6C'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'WORD PTR [RDI]' usage: USAGE_WRITE }
        operands { name: 'DX' usage: USAGE_READ }
      }
      raw_encoding_specification: '6D'
    })proto";
  TestTransform(FixOperandsOfInsAndOuts, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfInsAndOutsTest, Outs) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'OUTS'
        operands { name: 'DX' usage: USAGE_READ }
        operands { name: 'm16' usage: USAGE_READ }
      }
      raw_encoding_specification: '6F'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUTS'
        operands { name: 'DX' usage: USAGE_READ }
        operands { name: 'm32' usage: USAGE_READ }
      }
      raw_encoding_specification: '6F'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'OUTS'
        operands { name: 'DX' usage: USAGE_READ }
        operands { name: 'WORD PTR [RSI]' usage: USAGE_READ }
      }
      raw_encoding_specification: '6F'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'OUTS'
        operands { name: 'DX' usage: USAGE_READ }
        operands { name: 'DWORD PTR [RSI]' usage: USAGE_READ }
      }
      raw_encoding_specification: '6F'
    })proto";
  TestTransform(FixOperandsOfInsAndOuts, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLddquTest, FixOperands) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "LDDQU"
        operands {
          encoding: MODRM_REG_ENCODING
          name: "xmm1"
          usage: USAGE_WRITE
        }
        operands { encoding: MODRM_RM_ENCODING name: "mem" usage: USAGE_READ }
      }
      feature_name: "SSE3"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "F2 0F F0 /r"
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "LDDQU"
        operands {
          encoding: MODRM_REG_ENCODING
          name: "xmm1"
          usage: USAGE_WRITE
        }
        operands { encoding: MODRM_RM_ENCODING name: "m128" usage: USAGE_READ }
      }
      feature_name: "SSE3"
      available_in_64_bit: true
      legacy_instruction: true
      encoding_scheme: "RM"
      raw_encoding_specification: "F2 0F F0 /r"
    })proto";
  TestTransform(FixOperandsOfLddqu, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsScasAndStosTest, Scas) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AE'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'AL' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
        operands {
          name: 'BYTE PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'AE'
    })proto";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsScasAndStosTest, Stos) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AA'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm16' }
      }
      raw_encoding_specification: 'AB'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm32' }
      }
      raw_encoding_specification: 'AB'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'm64' }
      }
      raw_encoding_specification: 'REX.W + AB'
    }
    instructions {
      vendor_syntax { mnemonic: 'STOSB' }
      raw_encoding_specification: 'AA'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands {
          name: 'BYTE PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
        operands { name: 'AL' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
      }
      raw_encoding_specification: 'AA'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands {
          name: 'WORD PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
        operands { name: 'AX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
      }
      raw_encoding_specification: 'AB'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands {
          name: 'DWORD PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
        operands { name: 'EAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
      }
      raw_encoding_specification: 'AB'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands {
          name: 'QWORD PTR [RDI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
        operands { name: 'RAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
      }
      raw_encoding_specification: 'REX.W + AB'
    }
    instructions {
      vendor_syntax { mnemonic: 'STOSB' }
      raw_encoding_specification: 'AA'
    })proto";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfLodsAndStosTest, Lods) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: 'AC'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'm16' }
      }
      raw_encoding_specification: 'AD'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'm32' }
      }
      raw_encoding_specification: 'AD'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'm64' }
      }
      raw_encoding_specification: 'REX.W + AD'
    }
    instructions {
      vendor_syntax { mnemonic: 'LODSB' }
      raw_encoding_specification: 'AC'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'AL' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
        operands {
          name: 'BYTE PTR [RSI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'AC'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'AX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
        operands {
          name: 'WORD PTR [RSI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'AD'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'EAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
        operands {
          name: 'DWORD PTR [RSI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'AD'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LODS'
        operands { name: 'RAX' encoding: IMPLICIT_ENCODING usage: USAGE_READ }
        operands {
          name: 'QWORD PTR [RSI]'
          encoding: IMPLICIT_ENCODING
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: 'REX.W + AD'
    }
    instructions {
      vendor_syntax { mnemonic: 'LODSB' }
      raw_encoding_specification: 'AC'
    })proto";
  TestTransform(FixOperandsOfLodsScasAndStos, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfSgdtAndSidtTest, FixOperands) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "SGDT"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          name: "m"
          usage: USAGE_WRITE
        }
      }
      raw_encoding_specification: "0F 01 /0"
    }
    instructions {
      vendor_syntax {
        mnemonic: "INVLPG"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          name: "m"
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: "0F 01/7"
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "SGDT"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          name: "m16&64"
          usage: USAGE_WRITE
        }
      }
      raw_encoding_specification: "0F 01 /0"
    }
    instructions {
      vendor_syntax {
        mnemonic: "INVLPG"
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          name: "m"
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: "0F 01/7"
    })proto";
  TestTransform(FixOperandsOfSgdtAndSidt, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixOperandsOfVMovqTest, FixOperand) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'm64' }
      }
      raw_encoding_specification: 'VEX.128.F3.0F.WIG 7E /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'r/m64' }
      }
      raw_encoding_specification: 'VEX.128.66.0F.W1 6E /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: 'VEX.128.F3.0F.WIG 7E /r'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2/m64' }
      }
      raw_encoding_specification: 'VEX.128.F3.0F.WIG 7E /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'r/m64' }
      }
      raw_encoding_specification: 'VEX.128.66.0F.W1 6E /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVQ'
        operands { name: 'xmm1' }
        operands { name: 'xmm2/m64' }
      }
      raw_encoding_specification: 'VEX.128.F3.0F.WIG 7E /r'
    })proto";
  TestTransform(FixOperandsOfVMovq, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixRegOperandsTest, FixOperand) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'LAR'
        operands { name: 'r16' }
        operands { name: 'r16' }
      }
      raw_encoding_specification: '0F 02 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LAR'
        operands { name: 'reg' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: '0F 02 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVMSKPS'
        operands { name: 'reg' }
        operands { name: 'xmm' }
      }
      raw_encoding_specification: '0F 50 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVQ'
        operands { name: 'm64' }
        operands { name: 'mm' }
      }
      raw_encoding_specification: 'REX.W + 0F 7E /r'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'LAR'
        operands { name: 'r16' }
        operands { name: 'r16' }
      }
      raw_encoding_specification: '0F 02 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LAR'
        operands { name: 'r64' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: 'REX.W + 0F 02 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVMSKPS'
        operands { name: 'r32' }
        operands { name: 'xmm' }
      }
      raw_encoding_specification: '0F 50 /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOVQ'
        operands { name: 'm64' }
        operands { name: 'mm' }
      }
      raw_encoding_specification: 'REX.W + 0F 7E /r'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'LAR'
        operands { name: 'r32' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: '0F 02 /r'
    })proto";
  TestTransform(FixRegOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixRegOperandsTest, UnexpectedMnemonic) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'LARfoo'
        operands { name: 'reg' }
        operands { name: 'r32' }
      }
      raw_encoding_specification: '0F 02 /r'
    })proto";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(
      kInstructionSetProto, &instruction_set));
  const Status transform_status = FixRegOperands(&instruction_set);
  EXPECT_TRUE(IsInvalidArgument(transform_status)) << transform_status;
}

TEST(RemoveImplicitST0OperandTest, NoRemoval) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FCMOVE'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DA C8+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FCOM' }
      raw_encoding_specification: 'D8 D1'
    })proto";
  TestTransform(RemoveImplicitST0Operand, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(RemoveImplicitST0OperandTest, RemoveSomeOperands) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FCMOVE'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DA C8+i'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FADD'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'D8 C0+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FCOM' }
      raw_encoding_specification: 'D8 D1'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FCMOVE'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'DA C8+i'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FADD'
        operands { name: 'ST(i)' }
      }
      raw_encoding_specification: 'D8 C0+i'
    }
    instructions {
      vendor_syntax { mnemonic: 'FCOM' }
      raw_encoding_specification: 'D8 D1'
    })proto";
  TestTransform(RemoveImplicitST0Operand, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RemoveImplicitXmm0OperandTest, NoRemoval) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'VFMADD132PD'
        operands { name: 'xmm0' }
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: 'VEX.DDS.128.66.0F38.W1 98 /r'
    })proto";
  TestTransform(RemoveImplicitXmm0Operand, kInstructionSetProto,
                kInstructionSetProto);
}

// NOTE(ondrasej): All instructions using the implicit XMM0 use it as the last
// operand. Thus, we do not test any other case.
TEST(RemoveImplicitXmm0OperandTest, RemoveLastOperand) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'BLENDVPS'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
        operands { name: '<XMM0>' }
      }
      raw_encoding_specification: '66 0F 38 14 /r'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'BLENDVPS'
        operands { name: 'xmm1' }
        operands { name: 'xmm2' }
      }
      raw_encoding_specification: '66 0F 38 14 /r'
    })proto";
  TestTransform(RemoveImplicitXmm0Operand, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RenameOperandsTest, NoRenaming) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FADD'
        operands { name: 'ST(i)' }
        operands { name: 'ST(0)' }
      }
      raw_encoding_specification: 'DE C0+i'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'm8' }
        operands { name: 'r8' }
      }
      raw_encoding_specification: '88 /r'
    })proto";
  TestTransform(RenameOperands, kInstructionSetProto, kInstructionSetProto);
}

TEST(RenameOperandsTest, InstructionWithST) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FADD'
        operands { name: 'ST(i)' }
        operands { name: 'ST' }
      }
      raw_encoding_specification: 'DE C0+i'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FADD'
        operands { name: 'ST(i)' }
        operands { name: 'ST(0)' }
      }
      raw_encoding_specification: 'DE C0+i'
    })proto";
  TestTransform(RenameOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(RenameOperandsTest, InstructionWithM80Dec) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FBLD'
        operands { name: 'm80dec' }
      }
      raw_encoding_specification: 'DF /4'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FBLD'
        operands { name: 'm80bcd' }
      }
      raw_encoding_specification: 'DF /4'
    })proto";
  TestTransform(RenameOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
