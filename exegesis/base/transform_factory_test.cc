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

#include "exegesis/base/transform_factory.h"

#include "absl/flags/flag.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::util::OkStatus;

Status TestTransform1(InstructionSetProto*) { return OkStatus(); }
REGISTER_INSTRUCTION_SET_TRANSFORM(TestTransform1, kNotInDefaultPipeline);

Status TestTransform2(InstructionSetProto*) { return OkStatus(); }
REGISTER_INSTRUCTION_SET_TRANSFORM(TestTransform2, kNotInDefaultPipeline);

TEST(TransformFactoryTest, GetTransformsFromCommandLineFlags) {
  absl::SetFlag(&FLAGS_exegesis_transforms, "");
  EXPECT_EQ(GetTransformsFromCommandLineFlags().size(), 0);
  absl::SetFlag(&FLAGS_exegesis_transforms, "TestTransform1");
  EXPECT_EQ(GetTransformsFromCommandLineFlags().size(), 1);
  absl::SetFlag(&FLAGS_exegesis_transforms, "TestTransform2");
  EXPECT_EQ(GetTransformsFromCommandLineFlags().size(), 1);
  absl::SetFlag(&FLAGS_exegesis_transforms, "TestTransform1,TestTransform2");
  EXPECT_EQ(GetTransformsFromCommandLineFlags().size(), 2);
}

TEST(TransformFactoryDeathTest, GetTransformsFromCommandLineFlagsDoesNotExist) {
  absl::SetFlag(&FLAGS_exegesis_transforms, "DoesNotExist");
  EXPECT_DEATH(GetTransformsFromCommandLineFlags().size(), "");
}

}  // namespace
}  // namespace exegesis
