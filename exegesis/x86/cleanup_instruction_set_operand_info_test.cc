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

#include "exegesis/x86/cleanup_instruction_set_operand_info.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "exegesis/testing/test_util.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::StatusIs;
using ::exegesis::util::Status;
using ::exegesis::util::error::INVALID_ARGUMENT;

TEST(AddOperandInfoTest, AddInfo) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands { name: 'BYTE PTR [RDI]' }
        operands { name: 'AL' }
      }
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FMUL'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      x86_encoding_specification {
        legacy_prefixes {}
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVD'
        operands { name: 'xmm1' encoding: MODRM_REG_ENCODING }
        operands { name: 'r32' }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
        modrm_usage: FULL_MODRM
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STOS'
        operands {
          name: 'BYTE PTR [RDI]'
          addressing_mode: INDIRECT_ADDRESSING_BY_RDI
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
        }
        operands {
          name: 'AL'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 8
        }
      }
      x86_encoding_specification { legacy_prefixes {} }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'FMUL'
        operands {
          name: 'ST(0)'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
          value_size_bits: 80
        }
        operands {
          name: 'ST(i)'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 80
        }
      }
      x86_encoding_specification {
        legacy_prefixes {}
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'VMOVD'
        operands {
          name: 'xmm1'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
        }
        operands {
          name: 'r32'
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 32
        }
      }
      x86_encoding_specification {
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
          map_select: MAP_SELECT_0F
          vex_w_usage: VEX_W_IS_ZERO
          vex_operand_usage: VEX_OPERAND_IS_NOT_USED
        }
        modrm_usage: FULL_MODRM
      }
    })proto";
  TestTransform(AddOperandInfo, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddOperandInfoTest, DetectsInconsistentEncodings) {
  constexpr const char* const kInstructionSetProtos[] = {
      // The instruction encoding does not use the ModR/M byte, so the operands
      // can't use MODRM_RM_ENCODING.
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: 'STOS'
            operands { name: 'BYTE PTR [RDI]' encoding: MODRM_RM_ENCODING }
            operands { name: 'AL' }
          }
          x86_encoding_specification { legacy_prefixes {} }
        })proto",
      // Only one operand can be encoded in the opcode.
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: 'FMUL'
            operands { name: 'ST(0)' encoding: OPCODE_ENCODING }
            operands { name: 'ST(i)' encoding: MODRM_RM_ENCODING }
          }
          x86_encoding_specification {
            legacy_prefixes {}
            operand_in_opcode: FP_STACK_REGISTER_IN_OPCODE
          }
        })proto"};
  for (const char* const instruction_set_proto : kInstructionSetProtos) {
    InstructionSetProto instruction_set;
    ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(
        instruction_set_proto, &instruction_set));
    EXPECT_THAT(AddOperandInfo(&instruction_set), StatusIs(INVALID_ARGUMENT));
  }
}

TEST(AddVmxOperandInfoTest, NoArgs) {
  constexpr const char* const kInstructionSetProtos[] = {
      R"proto(
        instructions {
          vendor_syntax { mnemonic: "VMCALL" }
          syntax { mnemonic: "vmcall" }
          att_syntax { mnemonic: "vmcall" }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "0F 01 C1"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 983489
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_IGNORED
            }
          }
          instruction_group_index: 3
        }
      )proto",
      R"proto(
        instructions {
          vendor_syntax { mnemonic: "VMXOFF" }
          syntax { mnemonic: "vmxoff" }
          att_syntax { mnemonic: "vmxoff" }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "0F 01 C4"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 983492
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_IGNORED
            }
          }
          instruction_group_index: 10
        })proto"};

  for (const char* const instruction_set_proto : kInstructionSetProtos) {
    // The transform should not modify these because they are no-arg.
    TestTransform(AddVmxOperandInfo, instruction_set_proto,
                  instruction_set_proto);
  }
}

TEST(AddVmxOperandInfoTest, ArgsWithSuffix) {
  constexpr const char* const kInstructionSetProtos[] = {
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "VMCLEAR"
            operands {
              name: "m64"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "vmclear"
            operands { name: "qword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "vmclear"
            operands { name: "(%rsi)" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "66 0F C7 /r"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 4039
            modrm_usage: OPCODE_EXTENSION_IN_MODRM
            modrm_opcode_extension: 6
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_REQUIRED
            }
          }
          instruction_group_index: 4
        }
      )proto",
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "VMPTRLD"
            operands {
              name: "m64"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "vmptrld"
            operands { name: "qword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "vmptrld"
            operands { name: "(%rsi)" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "NP 0F C7 /6"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 4039
            modrm_usage: OPCODE_EXTENSION_IN_MODRM
            modrm_opcode_extension: 6
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_NOT_PERMITTED
            }
          }
          instruction_group_index: 6
        })proto"};

  for (const char* const instruction_set_proto : kInstructionSetProtos) {
    // The transform should not modify these because they have /6 suffix.
    TestTransform(AddVmxOperandInfo, instruction_set_proto,
                  instruction_set_proto);
  }
}

TEST(AddVmxOperandInfoTest, ArgsMissingSuffix) {
  constexpr int kLength = 2;
  constexpr const char* const kInstructionSetProtos[kLength] = {
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "INVEPT"
            operands {
              name: "r64"
              addressing_mode: DIRECT_ADDRESSING
              encoding: X86_REGISTER_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
              register_class: GENERAL_PURPOSE_REGISTER_64_BIT
            }
            operands {
              name: "m128"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 128
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "invept"
            operands { name: "r10" }
            operands { name: "xmmword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "invept"
            operands { name: "(%rsi)" }
            operands { name: "%r10" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "66 0F 38 80"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 997504
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_REQUIRED
            }
          }
          instruction_group_index: 1
        }
      )proto",
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "INVVPID"
            operands {
              name: "r64"
              addressing_mode: DIRECT_ADDRESSING
              encoding: X86_REGISTER_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
              register_class: GENERAL_PURPOSE_REGISTER_64_BIT
            }
            operands {
              name: "m128"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 128
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "invvpid"
            operands { name: "r10" }
            operands { name: "xmmword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "invvpid"
            operands { name: "(%rsi)" }
            operands { name: "%r10" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "66 0F 38 81"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 997505
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_REQUIRED
            }
          }
          instruction_group_index: 2
        }
      )proto"};

  constexpr const char* const kExpectedInstructionSetProtos[kLength] = {
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "INVEPT"
            operands {
              name: "r64"
              addressing_mode: DIRECT_ADDRESSING
              encoding: X86_REGISTER_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
              register_class: GENERAL_PURPOSE_REGISTER_64_BIT
            }
            operands {
              name: "m128"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 128
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "invept"
            operands { name: "r10" }
            operands { name: "xmmword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "invept"
            operands { name: "(%rsi)" }
            operands { name: "%r10" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "66 0F 38 80 /r"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 997504
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_REQUIRED
            }
          }
          instruction_group_index: 1
        }
      )proto",
      R"proto(
        instructions {
          vendor_syntax {
            mnemonic: "INVVPID"
            operands {
              name: "r64"
              addressing_mode: DIRECT_ADDRESSING
              encoding: X86_REGISTER_ENCODING
              value_size_bits: 64
              usage: USAGE_READ_WRITE
              register_class: GENERAL_PURPOSE_REGISTER_64_BIT
            }
            operands {
              name: "m128"
              addressing_mode: INDIRECT_ADDRESSING
              encoding: MODRM_RM_ENCODING
              value_size_bits: 128
              usage: USAGE_READ_WRITE
            }
          }
          syntax {
            mnemonic: "invvpid"
            operands { name: "r10" }
            operands { name: "xmmword ptr [rsi]" }
          }
          att_syntax {
            mnemonic: "invvpid"
            operands { name: "(%rsi)" }
            operands { name: "%r10" }
          }
          available_in_64_bit: true
          protection_mode: -1
          raw_encoding_specification: "66 0F 38 81 /r"
          feature_name: "VMX"
          x86_encoding_specification {
            opcode: 997505
            legacy_prefixes {
              rex_w_prefix: PREFIX_IS_IGNORED
              operand_size_override_prefix: PREFIX_IS_REQUIRED
            }
          }
          instruction_group_index: 2
        }
      )proto"};

  for (int i = 0; i < kLength; ++i) {
    const char* const instruction_set_proto = kInstructionSetProtos[i];
    const char* const expected_instruction_set_proto =
        kExpectedInstructionSetProtos[i];
    TestTransform(AddVmxOperandInfo, instruction_set_proto,
                  expected_instruction_set_proto);
  }
}

TEST(FixVmFuncOperandInfoTest, AddMissingInfo) {
  constexpr const char kVmCallProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "VMCALL" }
      syntax { mnemonic: "vmcall" }
      att_syntax { mnemonic: "vmcall" }
      available_in_64_bit: true
      protection_mode: -1
      raw_encoding_specification: "0F 01 C1"
      feature_name: "VMX"
      x86_encoding_specification {
        opcode: 983489
        legacy_prefixes {
          rex_w_prefix: PREFIX_IS_IGNORED
          operand_size_override_prefix: PREFIX_IS_IGNORED
        }
      }
      instruction_group_index: 3
    }
  )proto";
  constexpr const char kVmFuncProto[] = R"proto(
    instructions {
      description: "Invoke VMfunction specified in EAX."
      vendor_syntax { mnemonic: "VMFUNC" }
      feature_name: "VMX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 D4"
      instruction_group_index: 4
    }
  )proto";
  constexpr const char kExpectedTransformedVmFuncProto[] = R"proto(
    instructions {
      description: "Invoke VMfunction specified in EAX."
      vendor_syntax {
        mnemonic: "VMFUNC"
        operands {
          addressing_mode: ANY_ADDRESSING_WITH_FIXED_REGISTERS
          encoding: X86_REGISTER_EAX
          name: "EAX"
          usage: USAGE_READ
          description: "VM Function to be invoked."
        }
      }
      feature_name: "VMX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 D4"
      instruction_group_index: 4
    }
  )proto";

  // Functions other than VMFUNC should not be changed.
  TestTransform(FixVmFuncOperandInfo, kVmCallProto, kVmCallProto);

  // VMFUNC should be fixed.
  TestTransform(FixVmFuncOperandInfo, kVmFuncProto,
                kExpectedTransformedVmFuncProto);
}

TEST(AddMovdir64BOperandInfoTest, AddInfo) {
  constexpr const char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "MOVDIR64B"
        operands {
          encoding: MODRM_REG_ENCODING
          name: "r16/r32/r64"
          usage: USAGE_WRITE
        }
        operands { encoding: MODRM_RM_ENCODING name: "m512" usage: USAGE_READ }
      }
      raw_encoding_specification: "66 0F 38 F8 /r"
    }
    instructions {
      description: "Invoke VMfunction specified in EAX."
      vendor_syntax { mnemonic: "VMFUNC" }
      feature_name: "VMX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 D4"
      instruction_group_index: 4
    })proto";
  constexpr const char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "MOVDIR64B"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_WITH_BASE
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "m64"
          usage: USAGE_WRITE
        }
        operands { encoding: MODRM_RM_ENCODING name: "m512" usage: USAGE_READ }
      }
      raw_encoding_specification: "66 0F 38 F8 /r"
    }
    instructions {
      description: "Invoke VMfunction specified in EAX."
      vendor_syntax { mnemonic: "VMFUNC" }
      feature_name: "VMX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 D4"
      instruction_group_index: 4
    })proto";

  TestTransform(AddMovdir64BOperandInfo, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddUmonitorOperandInfoTest, AddInfo) {
  constexpr const char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "UMONITOR"
        operands {
          encoding: MODRM_RM_ENCODING
          name: "r16/r32/r64"
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: "F3 0F AE /6"
    })proto";
  constexpr const char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "UMONITOR"
        operands {
          addressing_mode: INDIRECT_ADDRESSING_WITH_BASE
          encoding: MODRM_RM_ENCODING
          value_size_bits: 8
          name: "mem"
          usage: USAGE_READ
        }
      }
      raw_encoding_specification: "F3 0F AE /6"
    })proto";
  TestTransform(AddUmonitorOperandInfo, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingOperandUsageTest, AddMissingOperandUsage) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STUFF'
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
        }
        operands { name: 'imm64' encoding: IMMEDIATE_VALUE_ENCODING }
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
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMPLICIT_ENCODING
          name: "1"
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
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
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMPLICIT_ENCODING
          name: "1"
          usage: USAGE_READ
        }
      }
    })proto";
  TestTransform(AddMissingOperandUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingOperandUsageToVblendInstructionsTest, AddMissingOperandUsage) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STUFF'
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
        }
        operands { name: 'imm64' encoding: IMMEDIATE_VALUE_ENCODING }
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VBLENDVPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
          usage: USAGE_WRITE
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 128
          name: "xmm2"
          usage: USAGE_READ
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_SUFFIX_ENCODING
          value_size_bits: 128
          name: "xmm4"
          register_class: VECTOR_REGISTER_128_BIT
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VBLENDVPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
          usage: USAGE_WRITE
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 128
          name: "xmm2"
          usage: USAGE_READ
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_SUFFIX_ENCODING
          value_size_bits: 128
          name: "xmm4"
          register_class: VECTOR_REGISTER_128_BIT
          usage: USAGE_READ_WRITE
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STUFF'
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
        }
        operands { name: 'imm64' encoding: IMMEDIATE_VALUE_ENCODING }
        operands {
          name: 'r64'
          addressing_mode: DIRECT_ADDRESSING
          encoding: IMPLICIT_ENCODING
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VBLENDVPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
          usage: USAGE_WRITE
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 128
          name: "xmm2"
          usage: USAGE_READ
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_SUFFIX_ENCODING
          value_size_bits: 128
          name: "xmm4"
          register_class: VECTOR_REGISTER_128_BIT
          usage: USAGE_READ
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VBLENDVPD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
          usage: USAGE_WRITE
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 128
          name: "xmm2"
          usage: USAGE_READ
          register_class: VECTOR_REGISTER_128_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_SUFFIX_ENCODING
          value_size_bits: 128
          name: "xmm4"
          register_class: VECTOR_REGISTER_128_BIT
          usage: USAGE_READ_WRITE
        }
      }
    })proto";
  TestTransform(AddMissingOperandUsageToVblendInstructions,
                kInstructionSetProto, kExpectedInstructionSetProto);
}

TEST(AddRegisterClassToOperandsTest, AddRegisterClassToOperands) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STUFF'
        operands { name: 'r64' }
        operands { name: 'imm64' }
        operands { name: 'm8' }
        operands { name: 'k' }
        operands { name: 'xmm1' }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'STUFF'
        operands { name: 'r64' register_class: GENERAL_PURPOSE_REGISTER_64_BIT }
        operands { name: 'imm64' }
        operands { name: 'm8' register_class: INVALID_REGISTER_CLASS }
        operands { name: 'k' register_class: MASK_REGISTER }
        operands { name: 'xmm1' register_class: VECTOR_REGISTER_128_BIT }
      }
    })proto";
  TestTransform(AddRegisterClassToOperands, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddMissingVexVOperandUsageTest, AddMissingVexOperandUsage) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "VMULPS"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
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
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "xmm3/m128"
          usage: USAGE_READ
        }
      }
      feature_name: "AVX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.128.0F.WIG 59 /r"
      x86_encoding_specification {
        opcode: 3929
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          map_select: MAP_SELECT_0F
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "BLSI"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_WRITE
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
      feature_name: "BMI1"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.NDD.LZ.0F38.W0 F3 /3"
      x86_encoding_specification {
        opcode: 997619
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 3
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "V4FMADDPS"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
          usage: USAGE_READ_WRITE
          register_class: VECTOR_REGISTER_512_BIT
        }
        operands {
          addressing_mode: BLOCK_DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 2048
          name: "zmm2+3"
          usage: USAGE_READ
          register_class: REGISTER_BLOCK_512_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
      }
      feature_name: "AVX512_4FMAPS"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "EVEX.DDS.512.F2.0F38.W0 9A /r"
      x86_encoding_specification {
        opcode: 997530
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_512_BIT
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
  )proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "VMULPS"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 128
          name: "xmm1"
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
          addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "xmm3/m128"
          usage: USAGE_READ
        }
      }
      feature_name: "AVX"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.128.0F.WIG 59 /r"
      x86_encoding_specification {
        opcode: 3929
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: VEX_PREFIX
          vector_size: VEX_VECTOR_SIZE_128_BIT
          map_select: MAP_SELECT_0F
          vex_operand_usage: VEX_OPERAND_IS_FIRST_SOURCE_REGISTER
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "BLSI"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 32
          name: "r32"
          usage: USAGE_WRITE
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
      feature_name: "BMI1"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "VEX.NDD.LZ.0F38.W0 F3 /3"
      x86_encoding_specification {
        opcode: 997619
        modrm_usage: OPCODE_EXTENSION_IN_MODRM
        modrm_opcode_extension: 3
        vex_prefix {
          prefix_type: VEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_DESTINATION_REGISTER
          vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "V4FMADDPS"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 512
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
          usage: USAGE_READ_WRITE
          register_class: VECTOR_REGISTER_512_BIT
        }
        operands {
          addressing_mode: BLOCK_DIRECT_ADDRESSING
          encoding: VEX_V_ENCODING
          value_size_bits: 2048
          name: "zmm2+3"
          usage: USAGE_READ
          register_class: REGISTER_BLOCK_512_BIT
        }
        operands {
          addressing_mode: INDIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 128
          name: "m128"
          usage: USAGE_READ
        }
      }
      feature_name: "AVX512_4FMAPS"
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "EVEX.DDS.512.F2.0F38.W0 9A /r"
      x86_encoding_specification {
        opcode: 997530
        modrm_usage: FULL_MODRM
        vex_prefix {
          prefix_type: EVEX_PREFIX
          vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
          vector_size: VEX_VECTOR_SIZE_512_BIT
          mandatory_prefix: MANDATORY_PREFIX_REPNE
          map_select: MAP_SELECT_0F38
          vex_w_usage: VEX_W_IS_ZERO
          opmask_usage: EVEX_OPMASK_IS_OPTIONAL
          masking_operation: EVEX_MASKING_MERGING_AND_ZEROING
        }
      }
    }
  )proto";

  TestTransform(AddMissingVexVOperandUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
