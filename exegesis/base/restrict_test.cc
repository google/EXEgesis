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

#include "exegesis/base/restrict.h"

#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;

class InstructionSetTest : public ::testing::Test {
 protected:
  InstructionSetTest()
      : instruction_set_(
            ParseProtoFromStringOrDie<InstructionSetProto>((R"proto(
              instructions { vendor_syntax { mnemonic: 'AAA' } }
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
                }
                encoding_scheme: 'NP'
                raw_encoding_specification: '6C'
              })proto"))) {}

  InstructionSetProto instruction_set_;
};

TEST_F(InstructionSetTest, RestrictToMnemonicRange) {
  RestrictToMnemonicRange("A", "INS", &instruction_set_);
  constexpr const char kExpected[] = R"proto(
    instructions { vendor_syntax { mnemonic: 'AAA' } }
    instructions {
      vendor_syntax {
        mnemonic: 'INS'
        operands { name: 'm8' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6C'
    })proto";
  EXPECT_THAT(instruction_set_, EqualsProto(kExpected));
}

TEST_F(InstructionSetTest, RestrictToMnemonicRangeNoop) {
  RestrictToMnemonicRange("A", "SCAS", &instruction_set_);
  constexpr const char kExpected[] = R"proto(
    instructions { vendor_syntax { mnemonic: 'AAA' } }
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
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: '6C'
    })proto";
  EXPECT_THAT(instruction_set_, EqualsProto(kExpected));
}

TEST_F(InstructionSetTest, RestrictToIndex) {
  RestrictToIndexRange(1, 2, &instruction_set_, nullptr);
  constexpr const char kExpected[] = R"proto(
    instructions {
      vendor_syntax {
        mnemonic: 'SCAS'
        operands { name: 'm8' }
      }
      encoding_scheme: 'NP'
      raw_encoding_specification: 'AE'
    }
  )proto";
  EXPECT_THAT(instruction_set_, EqualsProto(kExpected));
}

TEST_F(InstructionSetTest, RestrictToIndexRangeEmptyFront) {
  RestrictToIndexRange(0, 0, &instruction_set_, nullptr);
  EXPECT_TRUE(instruction_set_.instructions().empty());
}

TEST_F(InstructionSetTest, RestrictToIndexRangeEmptyEnd) {
  RestrictToIndexRange(15, 150, &instruction_set_, nullptr);
  EXPECT_TRUE(instruction_set_.instructions().empty());
}

}  // namespace
}  // namespace exegesis
