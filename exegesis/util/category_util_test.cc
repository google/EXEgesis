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

#include "exegesis/util/category_util.h"

#include <cstdint>

#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(InCategoryTest, TestCategories) {
  constexpr struct {
    int32_t value;
    int32_t category;
    bool expected_in_category;
  } kTestData[] = {
      {0x0, 0x0, true},       {0x1, 0x0, true},   {0xff, 0x0, true},
      {0x12, 0x1, true},      {0x12, 0x12, true}, {0x12345, 0x123, true},
      {0x2345, 0x123, false}, {0x2, 0x3, false},  {0x2, 0x23, false}};
  constexpr int kNumTestCases = 9;
  for (int i = 0; i < kNumTestCases; ++i) {
    SCOPED_TRACE(absl::StrCat("i = ", i));
    EXPECT_EQ(kTestData[i].expected_in_category,
              InCategory(kTestData[i].value, kTestData[i].category));
  }
}

}  // namespace
}  // namespace exegesis
