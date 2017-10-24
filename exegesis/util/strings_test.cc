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

#include "exegesis/util/strings.h"

#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace {

using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::ElementsAreArray;

void CheckInput(const std::string& hex_string,
                const std::vector<uint8_t>& expected_bytes) {
  SCOPED_TRACE(absl::StrCat("hex_string = ", hex_string));
  EXPECT_THAT(ParseHexString(hex_string),
              IsOkAndHolds(ElementsAreArray(expected_bytes)));
}

void CheckError(const std::string& hex_string,
                const std::string& expected_unparsed_part) {
  SCOPED_TRACE(absl::StrCat("hex_string = ", hex_string));
  EXPECT_THAT(ParseHexString(hex_string),
              StatusIs(INVALID_ARGUMENT, absl::StrCat("Could not parse: ",
                                                      expected_unparsed_part)));
}

TEST(ParseHexStringTest, TestEmptyString) { CheckInput("", {}); }

TEST(ParseHexStringTest, TestIntelManual) {
  CheckInput("AB BA F0 00", {0xab, 0xba, 0xf0, 0x0});
}

TEST(ParseHexStringTest, TestLowerCaseIntelManual) {
  CheckInput("ab ba f0 00", {0xab, 0xba, 0xf0, 0x0});
}

TEST(ParseHexStringTest, TestCPPArrayForamt) {
  CheckInput("0x0, 0x1, 0x2, 0xab", {0x0, 0x1, 0x2, 0xab});
}

TEST(ParseHexStringTest, TestCPPArrayWithNoSpaces) {
  CheckInput("0x0,0x1,0x2,0xab", {0x0, 0x1, 0x2, 0xab});
}

TEST(ParseHexStringTest, TestIntelManualWithCommas) {
  CheckInput("00,aB,Ba,cD, FF c0", {0x0, 0xab, 0xba, 0xcd, 0xff, 0xc0});
}

TEST(ParseHexStringTest, TestNonHexString) {
  CheckError("I'm not a hex std::string", "I'm not a hex std::string");
}

TEST(ParseHexStringTest, TestValidPrefix) {
  CheckError("0F AB blabla", "labla");
}

TEST(ToHumanReadableHexStringTest, EmptyVector) {
  const std::vector<uint8_t> kEmptyVector;
  EXPECT_TRUE(ToHumanReadableHexString(kEmptyVector).empty());
}

TEST(ToHumanReadableHexStringTest, FromVector) {
  const std::vector<uint8_t> kBinaryData = {0xab, 0xba, 0x1, 0x0};
  constexpr char kExpectedString[] = "AB BA 01 00";
  EXPECT_EQ(kExpectedString, ToHumanReadableHexString(kBinaryData));
}

TEST(ToHumanReadableHexStringTest, FromArray) {
  constexpr uint8_t kBinaryData[] = {0xab, 0xba, 0x1, 0x0};
  constexpr char kExpectedString[] = "AB BA 01 00";
  EXPECT_EQ(kExpectedString, ToHumanReadableHexString(kBinaryData));
}

TEST(ToPastableHexStringTest, EmptyVector) {
  const std::vector<uint8_t> kEmptyVector;
  EXPECT_TRUE(ToPastableHexString(kEmptyVector).empty());
}

TEST(ToPastableHexStringTest, FromVector) {
  const std::vector<uint8_t> kBinaryData = {0xab, 0xba, 0x1, 0x0};
  constexpr char kExpectedString[] = "0xAB, 0xBA, 0x01, 0x00";
  EXPECT_EQ(kExpectedString, ToPastableHexString(kBinaryData));
}

TEST(ToPastableHexStringTest, FromArray) {
  constexpr uint8_t kBinaryData[] = {0xab, 0xba, 0x1, 0x0};
  constexpr char kExpectedString[] = "0xAB, 0xBA, 0x01, 0x00";
  EXPECT_EQ(kExpectedString, ToPastableHexString(kBinaryData));
}

}  // namespace
}  // namespace exegesis
