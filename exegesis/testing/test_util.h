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

// Contains implementation of EqualsProto, a gMock matcher that takes a protocol
// buffer in the text format and matches it against "real" protocol buffers
// using the message differencer.
//
// Usage:
// MyProto proto = CreateMyProto();
// EXPECT_THAT(proto, EqualsProto("my_field: 'my_value'");

#ifndef EXEGESIS_TESTING_TEST_UTIL_H_
#define EXEGESIS_TESTING_TEST_UTIL_H_

#include <utility>
#include "strings/string.h"

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "src/google/protobuf/text_format.h"
#include "src/google/protobuf/util/message_differencer.h"

namespace exegesis {
namespace testing {
namespace internal {

template <typename ProtoType>
bool MatchProto(const ProtoType& actual_proto, const string& expected_proto_str,
                ::testing::MatchResultListener* listener);

}  // namespace internal

// A gMock matcher that takes a proto in the text format and compares protos
// against this text representation. Used to implement EqualsProto(str).
class EqualsProtoMatcher {
 public:
  explicit EqualsProtoMatcher(string expected_proto_str)
      : expected_proto_str_(std::move(expected_proto_str)) {}

  template <typename ProtoType>
  bool MatchAndExplain(const ProtoType& actual_proto,
                       ::testing::MatchResultListener* listener) const {
    return internal::MatchProto(actual_proto, expected_proto_str_, listener);
  }

  void DescribeTo(std::ostream* os) const {
    *os << "equals to proto:\n" << expected_proto_str_;
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not equal to proto:\n" << expected_proto_str_;
  }

 private:
  const string expected_proto_str_;
};

// Creates a polymorphic proto matcher based on the given proto in text format.
inline ::testing::PolymorphicMatcher<EqualsProtoMatcher> EqualsProto(
    string expected_proto_str) {
  return ::testing::MakePolymorphicMatcher(
      EqualsProtoMatcher(std::move(expected_proto_str)));
}

// Creates a polymorphic proto matcher based on the given proto.
inline ::testing::PolymorphicMatcher<EqualsProtoMatcher> EqualsProto(
    const google::protobuf::Message& expected_proto) {
  string expected_proto_str;
  using ::google::protobuf::TextFormat;
  CHECK(TextFormat::PrintToString(expected_proto, &expected_proto_str));
  return ::testing::MakePolymorphicMatcher(
      EqualsProtoMatcher(std::move(expected_proto_str)));
}

// A gMock matcher that takes a tuple containing a proto and a string containing
// a proto in text format, and compares the proto against the text
// representation. Used to implement EqualsProto().
class EqualsProtoTupleMatcher {
 public:
  EqualsProtoTupleMatcher() {}

  template <typename TupleType>
  bool MatchAndExplain(TupleType args,
                       ::testing::MatchResultListener* listener) const {
    using ::testing::get;
    return internal::MatchProto(get<0>(args), get<1>(args), listener);
  }

  void DescribeTo(std::ostream* os) const { *os << "are equal"; }

  void DescribeNegationTo(std::ostream* os) const { *os << "is not equal"; }
};

// Creates a tuple-based proto matcher that can be used e.g. with
// ::testing::Pointwise.
inline ::testing::PolymorphicMatcher<EqualsProtoTupleMatcher> EqualsProto() {
  return ::testing::MakePolymorphicMatcher(EqualsProtoTupleMatcher());
}

namespace internal {

// The implementation of the proto-to-string matching.
template <typename ProtoType>
bool MatchProto(const ProtoType& actual_proto, const string& expected_proto_str,
                ::testing::MatchResultListener* listener) {
  using ::google::protobuf::TextFormat;
  using ::google::protobuf::util::MessageDifferencer;
  ProtoType expected_proto;
  if (!TextFormat::ParseFromString(expected_proto_str, &expected_proto)) {
    *listener << "could not parse proto: <" << expected_proto_str << ">";
    return false;
  }

  MessageDifferencer differencer;
  string differences;
  differencer.ReportDifferencesToString(&differences);
  if (!differencer.Compare(expected_proto, actual_proto)) {
    *listener << "the protos are different:\n" << differences;
    return false;
  }

  return true;
}

}  // namespace internal
}  // namespace testing
}  // namespace exegesis

#endif  // EXEGESIS_TESTING_TEST_UTIL_H_
