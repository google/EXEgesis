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

#include "exegesis/util/status_util.h"

#include "absl/status/status.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::IsOk;
using ::exegesis::testing::StatusIs;
using ::testing::AllOf;
using ::testing::HasSubstr;

TEST(AnnotateStatusTest, AnnotateOkStatus) {
  const absl::Status status = AnnotateStatus(absl::OkStatus(), "Hello!");
  EXPECT_THAT(status, IsOk());
}

TEST(AnnotateStatusTest, AnnotateWithEmptyMessage) {
  const absl::Status status =
      AnnotateStatus(absl::NotFoundError("Not found"), "");
  EXPECT_THAT(status, StatusIs(absl::StatusCode::kNotFound, "Not found"));
}

TEST(AnnotateStatusTest, AnnotateWithNonEmptyMessage) {
  const absl::Status status = AnnotateStatus(
      absl::InvalidArgumentError("Ugly argument"), "Not so ugly");
  EXPECT_THAT(status, StatusIs(absl::StatusCode::kInvalidArgument,
                               AllOf(HasSubstr("Ugly argument"),
                                     HasSubstr("Not so ugly"))));
}

}  // namespace
}  // namespace exegesis
