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

#include "exegesis/x86/cleanup_instruction_set_properties.h"

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

TEST(AddMissingCpuFlagsTest, AddsMissing) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions { vendor_syntax { mnemonic: 'CLFLUSH' } }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: '6C'
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'CLFLUSH' }
      feature_name: 'CLFSH'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
      }
      raw_encoding_specification: '6C'
    })proto";
  TestTransform(AddMissingCpuFlags, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddRestrictedModesTest, AddsProtectionModes) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'HLT' }
      protection_mode: -1
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 64
          name: "CR0-CR7"
          usage: USAGE_READ
        }
      }
      available_in_64_bit: true
      raw_encoding_specification: "0F 20/r"
    }
    instructions {
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'r64' }
        operands { name: 'imm64' }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: 'HLT' }
      protection_mode: 0
    }
    instructions {
      vendor_syntax {
        mnemonic: "MOV"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_RM_ENCODING
          value_size_bits: 64
          name: "r64"
          usage: USAGE_WRITE
        }
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 64
          name: "CR0-CR7"
          usage: USAGE_READ
        }
      }
      protection_mode: 0
      available_in_64_bit: true
      raw_encoding_specification: "0F 20/r"
    }
    instructions {
      protection_mode: -1
      vendor_syntax {
        mnemonic: 'MOV'
        operands { name: 'r64' }
        operands { name: 'imm64' }
      }
    })proto";
  TestTransform(AddProtectionModes, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(FixAvailableIn64BitsTest, FixAvailability) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "AAD"
        operands { name: "imm8" }
      }
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "D5 ib"
    }
    instructions {
      vendor_syntax { mnemonic: "LAHF" }
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "9F"
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: "AAD"
        operands { name: "imm8" }
      }
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "D5 ib"
    }
    instructions {
      vendor_syntax { mnemonic: "LAHF" }
      legacy_instruction: true
      encoding_scheme: "ZO"
      raw_encoding_specification: "9F"
      available_in_64_bit: true
    })proto";
  TestTransform(FixAvailableIn64Bits, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
