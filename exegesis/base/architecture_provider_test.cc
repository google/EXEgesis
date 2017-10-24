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

#include "exegesis/base/architecture_provider.h"

#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;

constexpr const char kTestArchitectureProto[] = R"(
  name: "some_arch"
)";

TEST(ArchitectureProtoProviderTest, TestPbTxtSource) {
  const std::string filename =
      absl::StrCat(getenv("TEST_TMPDIR"), "/test_arch.pbtxt");
  WriteTextProtoOrDie(filename, ParseProtoFromStringOrDie<ArchitectureProto>(
                                    kTestArchitectureProto));
  EXPECT_THAT(
      *GetArchitectureProtoOrDie(absl::StrCat(kPbTxtSource, ":", filename)),
      EqualsProto(kTestArchitectureProto));
}

TEST(ArchitectureProtoProviderTest, TestPbSource) {
  const std::string filename =
      absl::StrCat(getenv("TEST_TMPDIR"), "/test_arch.pb");
  WriteBinaryProtoOrDie(filename, ParseProtoFromStringOrDie<ArchitectureProto>(
                                      kTestArchitectureProto));
  EXPECT_THAT(
      *GetArchitectureProtoOrDie(absl::StrCat(kPbSource, ":", filename)),
      EqualsProto(kTestArchitectureProto));
}

// A provider that returns an architecture with name equal to id.
class TestProvider : public ArchitectureProtoProvider {
 public:
  ~TestProvider() override {}

 private:
  std::shared_ptr<const ArchitectureProto> GetProtoOrDie() const override {
    const auto result = std::make_shared<ArchitectureProto>();
    result->set_name("an_id");
    return result;
  }
};
// The provider name has colons to check that splitting works as expected.
REGISTER_ARCHITECTURE_PROTO_PROVIDER("test:provider:with:colon", TestProvider);

TEST(ArchitectureProtoProviderTest, TestRegistration) {
  EXPECT_THAT(*GetArchitectureProtoOrDie(
                  absl::StrCat(kRegisteredSource, ":test:provider:with:colon")),
              EqualsProto("name: 'an_id'"));
}

TEST(ArchitectureProtoProviderDeathTest, UnknownSource) {
  ASSERT_DEATH(GetArchitectureProtoOrDie("unknown_source"), "unknown_source");
}

TEST(ArchitectureProtoProviderDeathTest, UnknownProvider) {
  ASSERT_DEATH(GetArchitectureProtoOrDie(
                   absl::StrCat(kRegisteredSource, ":does_not_exist")),
               "test:provider");
}

TEST(ArchitectureProtoProviderTest, TestRegisteredProviders) {
  EXPECT_THAT(GetRegisteredArchitectureIds(),
              ::testing::ElementsAre("test:provider:with:colon"));
}

}  // namespace
}  // namespace exegesis
