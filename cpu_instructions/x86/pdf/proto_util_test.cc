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

#include "cpu_instructions/x86/pdf/proto_util.h"

#include <cstdio>
#include "strings/string.h"

#include "cpu_instructions/testing/test_util.h"
#include "cpu_instructions/x86/pdf/pdf_document.pb.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"
#include "strings/str_cat.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {
namespace {

using ::cpu_instructions::testing::EqualsProto;

TEST(ProtoUtilTest, ReadWriteTextProtoOrDie) {
  constexpr char kExpected[] = R"(
    number: 1
    width: 612
    height: 792)";
  const PdfPage page = ParseProtoFromStringOrDie<PdfPage>(kExpected);
  const string filename = StrCat(getenv("TEST_TMPDIR"), "/test.pbtxt");
  WriteTextProtoOrDie(filename, page);
  const PdfPage read_page = ReadTextProtoOrDie<PdfPage>(filename);
  EXPECT_THAT(read_page, EqualsProto(kExpected));
}

TEST(ProtoUtilTest, ParseProtoFromStringOrDie) {
  EXPECT_THAT(ParseProtoFromStringOrDie<PdfPage>("number: 1"),
              EqualsProto("number: 1"));
}

TEST(ProtoUtilDeathTest, ParseProtoFromStringOrDie) {
  EXPECT_DEATH(ParseProtoFromStringOrDie<PdfPage>("doesnotexist: 1"), "");
}

}  // namespace
}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
