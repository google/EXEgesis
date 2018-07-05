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

#include "exegesis/x86/pdf/vendor_syntax.h"

#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace pdf {
namespace {

using ::exegesis::testing::EqualsProto;

TEST(ParseVendorSyntaxTest, Simple) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax("ADC r/m16, imm8", &vendor_syntax));
  constexpr char kExpected[] = R"proto(
    mnemonic: 'ADC'
    operands { name: 'r/m16' }
    operands { name: 'imm8' })proto";
  EXPECT_THAT(vendor_syntax, EqualsProto(kExpected));
}

TEST(ParseVendorSyntaxTest, AsterixAreRemoved) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax("ADC* r/m16*, imm8", &vendor_syntax));
  constexpr char kExpected[] = R"proto(
    mnemonic: 'ADC'
    operands { name: 'r/m16' }
    operands { name: 'imm8' })proto";
  EXPECT_THAT(vendor_syntax, EqualsProto(kExpected));
}

TEST(ParseVendorSyntaxTest, Prefix) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax("REP STOS m8", &vendor_syntax));
  constexpr char kExpected[] = R"proto(
    mnemonic: 'REP STOS'
    operands { name: 'm8' })proto";
  EXPECT_THAT(vendor_syntax, EqualsProto(kExpected));
}

TEST(ParseVendorSyntaxTest, Opmasks) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax(
      "VMULPD zmm1 {k1}{z}, zmm2, zmm3/m512/m64bcst{er}", &vendor_syntax));
  constexpr char kExpected[] = R"proto(
    mnemonic: 'VMULPD'
    operands {
      name: 'zmm1'
      tags { name: 'k1' }
      tags { name: 'z' }
    }
    operands { name: 'zmm2' }
    operands {
      name: 'zmm3/m512/m64bcst'
      tags { name: 'er' }
    })proto";
  EXPECT_THAT(vendor_syntax, EqualsProto(kExpected));
}

TEST(ParseVendorSyntaxTest, SimpleNoOperand) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax("ADC", &vendor_syntax));
  EXPECT_THAT(vendor_syntax, EqualsProto("mnemonic: 'ADC'"));
}

TEST(ParseVendorSyntaxTest, SimpleInvalidOperand) {
  InstructionFormat vendor_syntax;
  EXPECT_TRUE(ParseVendorSyntax("ADC r/m16, invalid_operand", &vendor_syntax));
  constexpr char kExpected[] = R"proto(
    mnemonic: 'ADC'
    operands { name: 'r/m16' }
    operands { name: '<UNKNOWN>' })proto";
  EXPECT_THAT(vendor_syntax, EqualsProto(kExpected));
}

TEST(ParseVendorSyntaxTest, Invalid) {
  InstructionFormat vendor_syntax;
  EXPECT_FALSE(ParseVendorSyntax("", &vendor_syntax));
  EXPECT_FALSE(ParseVendorSyntax("  , xmm0", &vendor_syntax));
}

}  // namespace
}  // namespace pdf
}  // namespace x86
}  // namespace exegesis
