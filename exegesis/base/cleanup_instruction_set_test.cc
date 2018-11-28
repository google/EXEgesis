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

#include "exegesis/base/cleanup_instruction_set.h"

#include <functional>

#include "exegesis/base/cleanup_instruction_set_test_utils.h"
#include "exegesis/testing/test_util.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace {

using ::exegesis::InstructionProto;
using ::exegesis::InstructionSetProto;
using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::TextFormat;

TEST(GetTransformsByNameTest, ReturnedMapIsNotEmpty) {
  const InstructionSetTransformsByName& transforms = GetTransformsByName();
  EXPECT_GT(transforms.size(), 0);
}

TEST(GetDefaultTransformPipelineTest, ReturnedVectorIsNotEmpty) {
  const std::vector<InstructionSetTransform> transforms =
      GetDefaultTransformPipeline();
  EXPECT_GT(transforms.size(), 0);
}

TEST(RunTransformWithDiffTest, NoDifference) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FMUL'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      feature_name: 'X87'
      raw_encoding_specification: 'D8 C8+i'
    })proto";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  // There is only one instruction in the proto, so SortByVendorSyntax leaves
  // the proto unchanged.
  EXPECT_THAT(RunTransformWithDiff(SortByVendorSyntax, &instruction_set),
              IsOkAndHolds(""));
}

// A dummy transform that deletes the second instruction in the instruction set,
// and returns Status::OK. Used for testing the diff.
Status DeleteSecondInstruction(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(instructions->begin() + 1);
  return OkStatus();
}

TEST(RunTransformWithDiffTest, WithDifference) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: 'AE'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6C'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm16' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6D'
    })proto";
  constexpr char kExpectedDiff[] =
      "deleted: instructions[1]: { vendor_syntax { mnemonic: \"INS\" operands "
      "{ name: \"m8\" } operands { name: \"DX\" } } encoding_scheme: \"NP\" "
      "raw_encoding_specification: \"6C\" }\n";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  EXPECT_THAT(RunTransformWithDiff(DeleteSecondInstruction, &instruction_set),
              IsOkAndHolds(kExpectedDiff));
}

// A dummy transform that immediately returns an error.
Status ReturnErrorInsteadOfTransforming(InstructionSetProto* instruction_set) {
  return InvalidArgumentError("I do not transform!");
}

TEST(RunTransformWithDiffTest, WithError) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'FMUL'
        operands { name: 'ST(0)' }
        operands { name: 'ST(i)' }
      }
      feature_name: 'X87'
      raw_encoding_specification: 'D8 C8+i'
    })proto";
  InstructionSetProto instruction_set;
  ASSERT_TRUE(
      TextFormat::ParseFromString(kInstructionSetProto, &instruction_set));
  EXPECT_THAT(
      RunTransformWithDiff(ReturnErrorInsteadOfTransforming, &instruction_set),
      StatusIs(INVALID_ARGUMENT, "I do not transform!"));
}

TEST(SortByVendorSyntaxTest, Sort) {
  constexpr char kInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: 'AE'
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
        }
        operands { name: "zmm2" }
        operands {
          name: "m512"
          tags { name: "er" }
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          name: "zmm1"
          tags { name: "k1" }
        }
        operands { name: "zmm2" }
        operands {
          name: "m512"
          tags { name: "er" }
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6C'
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm1" }
        operands { name: "zmm2" }
        operands { name: "m512" }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm2" }
        operands { name: "zmm3" }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm1" }
        operands { name: "zmm2" }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm16' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6D'
    }
    instructions {
      vendor_syntax { mnemonic: "ENCLS" }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 CF"
      leaf_instructions {
        vendor_syntax {
          mnemonic: "EAUG"
          operands {
            encoding: X86_REGISTER_EAX
            name: "EAX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "EAUG (In)"
            value: "\r"
          }
          operands {
            encoding: X86_REGISTER_RBX
            name: "RBX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of a SECINFO (In)"
          }
          operands {
            encoding: X86_REGISTER_RCX
            name: "RCX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of the destination EPC page (In)"
          }
        }
      }
      leaf_instructions {
        vendor_syntax {
          mnemonic: "EADD"
          operands {
            encoding: X86_REGISTER_EAX
            name: "EAX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "EADD (In)"
            value: "\001"
          }
          operands {
            encoding: X86_REGISTER_RBX
            name: "RBX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of a PAGEINFO (In)"
          }
          operands {
            encoding: X86_REGISTER_RCX
            name: "RCX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of the destination EPC page (In)"
          }
        }
      }
    })proto";
  constexpr char kExpectedInstructionSetProto[] = R"proto(
    instructions {
      vendor_syntax { mnemonic: "ENCLS" }
      available_in_64_bit: true
      legacy_instruction: true
      raw_encoding_specification: "NP 0F 01 CF"
      leaf_instructions {
        vendor_syntax {
          mnemonic: "EADD"
          operands {
            encoding: X86_REGISTER_EAX
            name: "EAX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "EADD (In)"
            value: "\001"
          }
          operands {
            encoding: X86_REGISTER_RBX
            name: "RBX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of a PAGEINFO (In)"
          }
          operands {
            encoding: X86_REGISTER_RCX
            name: "RCX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of the destination EPC page (In)"
          }
        }
      }
      leaf_instructions {
        vendor_syntax {
          mnemonic: "EAUG"
          operands {
            encoding: X86_REGISTER_EAX
            name: "EAX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "EAUG (In)"
            value: "\r"
          }
          operands {
            encoding: X86_REGISTER_RBX
            name: "RBX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of a SECINFO (In)"
          }
          operands {
            encoding: X86_REGISTER_RCX
            name: "RCX"
            data_type { kind: INTEGER bit_width: 64 }
            description: "Address of the destination EPC page (In)"
          }
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm16' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6D'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
        operands { name: 'DX' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6C'
    }
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: 'AE'
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm1" }
        operands { name: "zmm2" }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm1" }
        operands { name: "zmm2" }
        operands { name: "m512" }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          name: "zmm1"
          tags { name: "k1" }
        }
        operands { name: "zmm2" }
        operands {
          name: "m512"
          tags { name: "er" }
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands {
          name: "zmm1"
          tags { name: "k1" }
          tags { name: "z" }
        }
        operands { name: "zmm2" }
        operands {
          name: "m512"
          tags { name: "er" }
        }
      }
    }
    instructions {
      vendor_syntax {
        mnemonic: "VADDPD"
        operands { name: "zmm2" }
        operands { name: "zmm3" }
      }
    })proto";
  TestTransform(SortByVendorSyntax, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace exegesis
