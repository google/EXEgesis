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

#include "exegesis/util/bits.h"

#include <cstdint>

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(BitsTest, IsNthBitSet) {
  constexpr struct {
    uint32_t value;
    int bit_position;
    bool expected_is_set;
  } kTestCases[] = {{0, 0, false},     {0, 15},           {0, 31, false},
                    {1, 0, true},      {1, 1, false},     {1, 15, false},
                    {0xf00, 0, false}, {0xf00, 7, false}, {0xf00, 8, true},
                    {0xf00, 11, true}, {0xf00, 12, false}};
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat("test_case = {", test_case.value, ", ",
                              test_case.bit_position, ", ",
                              test_case.expected_is_set, "}"));
    const bool is_set = IsNthBitSet(test_case.value, test_case.bit_position);
    EXPECT_EQ(is_set, test_case.expected_is_set);
  }
}

#ifndef NDEBUG
// We use DCHECK macros in the code, so the death tests work only in debug mode.
TEST(BitsDeathTest, IsNthBitSet) {
  EXPECT_DEATH(IsNthBitSet(0, -1), "");
  EXPECT_DEATH(IsNthBitSet(0, 32), "");
  EXPECT_DEATH(IsNthBitSet(0, 64), "");
}
#endif  // NDEBUG

TEST(BitsTest, ClearBitRange) {
  constexpr struct {
    uint32_t value;
    int start_bit_position;
    int end_bit_position;
    uint32_t expected_result;
  } kTestCases[] = {{0, 0, 32, 0},
                    {0xffffffff, 0, 32, 0},
                    {0xffffffff, 0, 8, 0xffffff00},
                    {0xffffffff, 8, 16, 0xffff00ff},
                    {0xabcdef01, 24, 32, 0xcdef01}};
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat("test_case = {", test_case.value, ", ",
                              test_case.start_bit_position, ", ",
                              test_case.end_bit_position, ", ",
                              test_case.expected_result, "}"));
    const uint32_t result =
        ClearBitRange(test_case.value, test_case.start_bit_position,
                      test_case.end_bit_position);
    EXPECT_EQ(result, test_case.expected_result);
  }
}

#ifndef NDEBUG
// We use DCHECK macros in the code, so the death tests work only in fastbuild
// and debug modes.
TEST(BitsDeathTest, ClearBitRange) {
  EXPECT_DEATH(ClearBitRange(0, -1, 4), "");
  EXPECT_DEATH(ClearBitRange(0, 0, 33), "");
  EXPECT_DEATH(ClearBitRange(0, 5, 4), "");
  EXPECT_DEATH(ClearBitRange(0, 5, 5), "");
}
#endif  // NDEBUG

TEST(BitsTest, GetBitRange) {
  constexpr struct {
    uint32_t value;
    int start_bit_position;
    int end_bit_position;
    uint32_t expected_bit_range;
  } kTestCases[] = {{0, 0, 16, 0},
                    {0xf0f0f0f, 4, 12, 0xf0},
                    {0xabcdef89, 24, 32, 0xab},
                    {0xabcdef89, 0, 32, 0xabcdef89}};
  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(absl::StrCat(
        "test_case = {", test_case.value, ", ", test_case.start_bit_position,
        ", ", test_case.end_bit_position, ", ", test_case.expected_bit_range));
    const uint32_t bit_range =
        GetBitRange(test_case.value, test_case.start_bit_position,
                    test_case.end_bit_position);
    EXPECT_EQ(bit_range, test_case.expected_bit_range);
  }
}

#ifndef NDEBUG
// We use DCHECK macros in the code, so the death tests work only in fastbuild
// and debug modes.
TEST(BitsDeathTest, GetBitRange) {
  EXPECT_DEATH(GetBitRange(0, -1, 4), "");
  EXPECT_DEATH(GetBitRange(0, 0, 33), "");
  EXPECT_DEATH(GetBitRange(0, 7, 4), "");
  EXPECT_DEATH(GetBitRange(0, 7, 7), "");
}
#endif  // NDEBUG

}  // namespace
}  // namespace exegesis
