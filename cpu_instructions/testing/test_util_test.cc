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

#include "cpu_instructions/testing/test_util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "cpu_instructions/testing/test.pb.h"

namespace cpu_instructions {
namespace testing {
namespace {

using ::testing::Pointwise;
using ::testing::StringMatchResultListener;

TEST(EquasProtoMatcherTest, EqualsString) {
  TestProto actual_proto;
  actual_proto.set_integer_field(1);
  actual_proto.set_string_field("blabla");
  EXPECT_THAT(actual_proto,
              EqualsProto("integer_field: 1 string_field: 'blabla'"));
}

TEST(EquasProtoMatcherTest, EqualsProto) {
  TestProto actual_proto;
  actual_proto.set_integer_field(1);
  actual_proto.set_string_field("blabla");
  TestProto expected_proto;
  expected_proto.set_integer_field(1);
  expected_proto.set_string_field("blabla");
  EXPECT_THAT(actual_proto, EqualsProto(expected_proto));
}

TEST(EqualsProtoMatcherTest, InvalidExpectedProto) {
  TestProto actual_proto;
  EqualsProtoMatcher matcher("foobar!");
  StringMatchResultListener listener;
  EXPECT_FALSE(matcher.MatchAndExplain(actual_proto, &listener));
  EXPECT_EQ(listener.str(), "could not parse proto: <foobar!>");
}

TEST(EqualsProtoMatcherTest, DifferentProtos) {
  TestProto actual_proto;
  actual_proto.set_integer_field(1);
  EqualsProtoMatcher matcher("integer_field: 2");
  StringMatchResultListener listener;
  EXPECT_FALSE(matcher.MatchAndExplain(actual_proto, &listener));
  EXPECT_EQ(listener.str(),
            "the protos are different:\nmodified: integer_field: 2 -> 1\n");
}

TEST(EqualsProtoTupleMatcherTest, Pointwise) {
  std::vector<TestProto> actual_protos(3);
  actual_protos[0].set_integer_field(1);
  actual_protos[1].set_string_field("hello");
  actual_protos[2].set_integer_field(2);
  actual_protos[2].set_string_field("world");
  const std::vector<string> expected_protos = {
      "integer_field: 1", "string_field: 'hello'",
      "integer_field: 2 string_field: 'world'"};
  EXPECT_THAT(actual_protos, Pointwise(EqualsProto(), expected_protos));
}

TEST(EqualsProtoTupleMatcherTest, InvalidExpectedProto) {
  const TestProto actual_proto;
  const string expected_proto = "foobar!";
  auto tuple = ::testing::make_tuple(actual_proto, expected_proto);
  EqualsProtoTupleMatcher matcher;
  StringMatchResultListener listener;
  EXPECT_FALSE(matcher.MatchAndExplain(tuple, &listener));
  EXPECT_EQ(listener.str(), "could not parse proto: <foobar!>");
}

}  // namespace
}  // namespace testing
}  // namespace cpu_instructions
