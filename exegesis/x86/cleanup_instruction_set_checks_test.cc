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

#include "exegesis/x86/cleanup_instruction_set_checks.h"

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::IsOk;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::Status;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::HasSubstr;
using ::testing::Matcher;
using ::testing::StartsWith;

TEST(CheckOpcodeFormatTest, ValidOpcodes) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "ADC"
        operands { name: "AL" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "14 ib"
      x86_encoding_specification {
        opcode: 0x14
        legacy_prefixes { operand_size_override_prefix: PREFIX_IS_IGNORED }
        immediate_value_bytes: 1
      }
    }
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
    }
    instructions {
      vendor_syntax {
        mnemonic: "KSHIFTLB"
        operands { name: "k1" }
        operands { name: "k2" }
        operands { name: "imm8" }
      }
      raw_encoding_specification: "VEX.L0.66.0F3A.W0 32 /r ib"
      x86_encoding_specification {
        opcode: 0x0f3a32
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
    })proto";
  InstructionSetProto instruction_set =
      ParseProtoFromStringOrDie<InstructionSetProto>(kInstructionSetProto);
  EXPECT_OK(CheckOpcodeFormat(&instruction_set));
  EXPECT_THAT(instruction_set, EqualsProto(kInstructionSetProto));
}

TEST(CheckOpcodeFormatTest, InvalidOpcodes) {
  static constexpr struct {
    const char* instruction_set;
    const char* expected_error_message;
  } kTestCases[] =  //
      {{R"proto(instructions {
                  vendor_syntax {
                    mnemonic: 'FSUB'
                    operands { name: 'ST(i)' }
                  }
                  raw_encoding_specification: 'D8 E0+i'
                  x86_encoding_specification {
                    opcode: 55520
                    operand_in_opcode: FP_STACK_REGISTER_IN_OPCODE
                    legacy_prefixes {
                      operand_size_override_prefix: PREFIX_IS_IGNORED
                    }
                  }
                })proto",
        "Invalid opcode upper bytes: d800"},
       {R"proto(instructions {
                  vendor_syntax { mnemonic: "XTEST" }
                  raw_encoding_specification: "NP 0F 01 D6"
                  x86_encoding_specification {
                    opcode: 983510
                    legacy_prefixes {
                      operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
                    }
                  }
                })proto",
        "Invalid opcode upper bytes: f0100"}};
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(
            test_case.instruction_set);
    EXPECT_THAT(CheckOpcodeFormat(&instruction_set),
                StatusIs(INVALID_ARGUMENT,
                         HasSubstr(test_case.expected_error_message)));
  }
}

TEST(CheckOperandInfoTest, CheckInstructions) {
  static const struct {
    const char* instruction_set;
    Matcher<Status> expected_status;
  } kTestCases[] = {
      {// A valid instruction.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     name: "m64"
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 64
                     name: "r64"
                     usage: USAGE_READ
                     register_class: GENERAL_PURPOSE_REGISTER_64_BIT
                   }
                 }
               })proto",
       IsOk()},
      {// LOAD_EFFECTIVE_ADDRESS operands do not require value_size_bits.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "INVLPG"
                   operands {
                     addressing_mode: LOAD_EFFECTIVE_ADDRESS
                     encoding: MODRM_RM_ENCODING
                     name: "m"
                     usage: USAGE_READ
                   }
                 }
               })proto",
       IsOk()},
      {// Pseudo-operands do not trigger warnings about an empty name or missing
       // value size bits.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "VCMPPD"
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     name: "k1"
                     tags { name: "k2" }
                     usage: USAGE_WRITE
                     register_class: MASK_REGISTER
                     value_size_bits: 64
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: VEX_V_ENCODING
                     value_size_bits: 512
                     name: "zmm2"
                     usage: USAGE_READ
                     register_class: VECTOR_REGISTER_512_BIT
                   }
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 512
                     name: "m512"
                     usage: USAGE_READ
                   }
                   operands {
                     addressing_mode: NO_ADDRESSING
                     encoding: X86_STATIC_PROPERTY_ENCODING
                     tags { name: "sae" }
                     usage: USAGE_READ
                   }
                   operands {
                     addressing_mode: NO_ADDRESSING
                     encoding: IMMEDIATE_VALUE_ENCODING
                     value_size_bits: 8
                     name: "imm8"
                     usage: USAGE_READ
                   }
                 }
                 raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 C2 /r ib"
               })proto",
       IsOk()},
      {// Missing operand encoding.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     name: "m64"
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     value_size_bits: 64
                     name: "r64"
                     usage: USAGE_READ
                     register_class: GENERAL_PURPOSE_REGISTER_64_BIT
                   }
                 }
               })proto",
       StatusIs(INVALID_ARGUMENT,
                StartsWith("Operand encoding in InstructionProto.vendor_syntax "
                           "is not set"))},
      {// Missing operand addressing mode.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     name: "m64"
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 64
                     name: "r64"
                     usage: USAGE_READ
                     register_class: GENERAL_PURPOSE_REGISTER_64_BIT
                   }
                 }
               })proto",
       StatusIs(INVALID_ARGUMENT,
                StartsWith("Addressing mode in InstructionProto.vendor_syntax "
                           "is not set"))},
      {// Missing operand name + no tags.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 64
                     name: "r64"
                     usage: USAGE_READ
                     register_class: GENERAL_PURPOSE_REGISTER_64_BIT
                   }
                 }
               })proto",
       StatusIs(INVALID_ARGUMENT,
                StartsWith("Operand name or tags in "
                           "InstructionProto.vendor_syntax are not valid"))},
      {// Missing register class.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     name: "m64"
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 64
                     name: "r64"
                     usage: USAGE_READ
                   }
                 }
               })proto",
       StatusIs(
           INVALID_ARGUMENT,
           StartsWith(
               "Register class in InstructionProto.vendor_syntax is not set"))},
      {// Missing usage.
       R"proto(instructions {
                 vendor_syntax {
                   mnemonic: "ADC"
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 64
                     name: "m64"
                     usage: USAGE_READ_WRITE
                   }
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 64
                     name: "r64"
                     register_class: GENERAL_PURPOSE_REGISTER_64_BIT
                   }
                 }
               })proto",
       StatusIs(
           INVALID_ARGUMENT,
           StartsWith(
               "Operand usage in InstructionProto.vendor_syntax is not set"))},
  };
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(
            test_case.instruction_set);
    EXPECT_THAT(CheckOperandInfo(&instruction_set), test_case.expected_status);
  }
}

TEST(CheckSpecialCaseInstructionsTest, CoveredInstructions) {
  static constexpr struct {
    const char* instruction_set;
    const char* expected_error_message;
  } kTestCases[] = {
      {R"proto(instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification {
                   opcode: 0xD8
                   modrm_usage: OPCODE_EXTENSION_IN_MODRM
                   modrm_opcode_extension: 0x4
                 }
               }
               instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification { opcode: 0xD8E1 }
               })proto",
       "Opcode is ambigious: d8e1"},
      {R"proto(instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification {
                   opcode: 0xD8
                   modrm_usage: OPCODE_EXTENSION_IN_MODRM
                   modrm_opcode_extension: 0x4
                 }
               }
               instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: INDIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification { opcode: 0xD861 }
               })proto",
       "Opcode is ambigious: d861"},
      {R"proto(instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification {
                   opcode: 0xD9
                   modrm_usage: OPCODE_EXTENSION_IN_MODRM
                   modrm_opcode_extension: 0x2
                 }
               }
               instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification { opcode: 0xD9D4 }
               })proto",
       "Opcode is ambigious: d9d4"},
      {R"proto(instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: OPCODE_ENCODING
                   }
                 }
                 x86_encoding_specification { opcode: 0xD9 }
               }
               instructions {
                 vendor_syntax {
                   operands {
                     addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_RM_ENCODING
                   }
                 }
                 x86_encoding_specification { opcode: 0xD964 }
               })proto",
       "Opcode is ambigious: d964"},
  };
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(
            test_case.instruction_set);
    EXPECT_THAT(CheckSpecialCaseInstructions(&instruction_set),
                StatusIs(INVALID_ARGUMENT,
                         HasSubstr(test_case.expected_error_message)));
  }
}

TEST(CheckSpecialCaseInstructions, NotCoveredInstructions) {
  static constexpr const char* kTestCases[] =  //
      {
          R"proto(instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification {
                      opcode: 0xD8
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 0x4
                    }
                  }
                  instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification { opcode: 0xD8F1 }
                  })proto",
          R"proto(instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification {
                      opcode: 0xD9
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 0x2
                    }
                  }
                  instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification { opcode: 0xD9E4 }
                  })proto",
          R"proto(instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: INDIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification {
                      opcode: 0xD8
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 0x5
                    }
                  }
                  instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: INDIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification { opcode: 0xD861 }
                  })proto",
          R"proto(instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: INDIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification {
                      opcode: 0xD8
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 0x5
                    }
                  }
                  instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: INDIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification { opcode: 0xD361 }
                  })proto",
          R"proto(instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification {
                      opcode: 0xD9
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 0x2
                    }
                  }
                  instructions {
                    vendor_syntax {
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    x86_encoding_specification { opcode: 0xD964 }
                  })proto",
          R"proto(instructions {
                    vendor_syntax { mnemonic: "MFENCE" }
                    raw_encoding_specification: "NP 0F AE F0"
                    x86_encoding_specification {
                      opcode: 1027824
                      legacy_prefixes {
                        rex_w_prefix: PREFIX_IS_IGNORED
                        operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
                      }
                    }
                  }
                  instructions {
                    vendor_syntax {
                      mnemonic: "TPAUSE"
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                      }
                    }
                    raw_encoding_specification: "66 0F AE /6"
                    x86_encoding_specification {
                      opcode: 4014
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 6
                      legacy_prefixes {
                        rex_w_prefix: PREFIX_IS_NOT_PERMITTED
                        operand_size_override_prefix: PREFIX_IS_REQUIRED
                      }
                    }
                  })proto",
          R"proto(instructions {
                    vendor_syntax {
                      mnemonic: "UMWAIT"
                      operands {
                        addressing_mode: DIRECT_ADDRESSING
                        encoding: MODRM_RM_ENCODING
                        value_size_bits: 32
                        name: "r32"
                        usage: USAGE_READ
                        register_class: GENERAL_PURPOSE_REGISTER_32_BIT
                      }
                    }
                    feature_name: "WAITPKG"
                    available_in_64_bit: true
                    legacy_instruction: true
                    encoding_scheme: "A"
                    raw_encoding_specification: "F2 0F AE /6"
                    protection_mode: -1
                    x86_encoding_specification {
                      opcode: 4014
                      modrm_usage: OPCODE_EXTENSION_IN_MODRM
                      modrm_opcode_extension: 6
                      legacy_prefixes {
                        has_mandatory_repne_prefix: true
                        rex_w_prefix: PREFIX_IS_NOT_PERMITTED
                        operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
                      }
                    }
                  }
                  instructions {
                    vendor_syntax { mnemonic: "MFENCE" }
                    available_in_64_bit: true
                    legacy_instruction: true
                    encoding_scheme: "ZO"
                    raw_encoding_specification: "NP 0F AE F0"
                    protection_mode: -1
                    x86_encoding_specification {
                      opcode: 1027824
                      legacy_prefixes {
                        rex_w_prefix: PREFIX_IS_IGNORED
                        operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
                      }
                    }
                  })proto",
      };
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(test_case);
    EXPECT_OK(CheckSpecialCaseInstructions(&instruction_set));
  }
}

TEST(CheckHasVendorSyntaxTest, CheckSyntax) {
  const struct {
    const char* instruction_set;
    Matcher<Status> status_matcher;
  } kTestCases[] = {
      {"", IsOk()},
      {R"proto(
         instructions {
           vendor_syntax {
             operands {
               addressing_mode: INDIRECT_ADDRESSING
               encoding: MODRM_RM_ENCODING
             }
           }
           x86_encoding_specification {
             opcode: 0xD8
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 0x5
           }
         }
         instructions {
           vendor_syntax {
             operands {
               addressing_mode: INDIRECT_ADDRESSING
               encoding: MODRM_RM_ENCODING
             }
           }
           x86_encoding_specification { opcode: 0xD361 }
         }
       )proto",
       IsOk()},
      {R"proto(
         instructions {
           vendor_syntax {
             operands {
               addressing_mode: INDIRECT_ADDRESSING
               encoding: MODRM_RM_ENCODING
             }
           }
           x86_encoding_specification {
             opcode: 0xD8
             modrm_usage: OPCODE_EXTENSION_IN_MODRM
             modrm_opcode_extension: 0x5
           }
         }
         instructions { x86_encoding_specification { opcode: 0xD361 } }
       )proto",
       StatusIs(INVALID_ARGUMENT)},
  };
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(
            test_case.instruction_set);
    EXPECT_THAT(CheckHasVendorSyntax(&instruction_set),
                test_case.status_matcher);
  }
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
