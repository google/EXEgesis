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

#include "exegesis/x86/operand_translator.h"

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;

TEST(OperandTranslatorTest, Works) {
  const auto instruction = ParseProtoFromStringOrDie<InstructionProto>(R"(
      legacy_instruction: true
      vendor_syntax {
        mnemonic: "ADD"
        operands {
          addressing_mode: DIRECT_ADDRESSING
          encoding: MODRM_REG_ENCODING
          value_size_bits: 32
          name: "r32" }
        operands {
          addressing_mode: NO_ADDRESSING
          encoding: IMMEDIATE_VALUE_ENCODING
          value_size_bits: 8
          name: "imm8"
        }
      })");
  constexpr const char kExpected[] = R"(
      mnemonic: "ADD"
      operands { name: "ecx" }
      operands { name: "0x7e" }
  )";
  EXPECT_THAT(InstantiateOperands(instruction), EqualsProto(kExpected));
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
