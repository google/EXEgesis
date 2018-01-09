// Copyright 2018 Google Inc.
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

#include "exegesis/base/opcode.h"

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(OpcodeTest, Comparison) {
  Opcode add_1(0x0);
  Opcode add_2(0x0);
  Opcode nop(0x90);

  EXPECT_EQ(add_1, add_2);
  EXPECT_NE(add_1, nop);
  EXPECT_LT(add_1, nop);
  EXPECT_LE(add_1, nop);
  EXPECT_LE(add_1, add_2);
  EXPECT_GT(nop, add_1);
  EXPECT_GE(nop, add_1);
  EXPECT_GE(nop, nop);
}

TEST(OpcodeTest, ToString) {
  constexpr struct {
    Opcode opcode;
    const char* expected_string;
  } kTestCases[] = {{Opcode(0x0), "00"},
                    {Opcode(0x90), "90"},
                    {Opcode(0x0f06), "0F 06"},
                    {Opcode(0x0f3898), "0F 38 98"}};
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat("expected_string = ", test_case.expected_string));
    const std::string formatted_opcode = test_case.opcode.ToString();
    EXPECT_EQ(formatted_opcode, test_case.expected_string);
  }
}

}  // namespace
}  // namespace exegesis
