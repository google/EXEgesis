#include "exegesis/base/architecture_provider.h"

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "strings/str_cat.h"

namespace exegesis {
namespace {

using testing::EqualsProto;

constexpr const char kTestArchitectureProto[] = R"(
  name: "some_arch"
)";

TEST(ArchitectureProtoProviderTest, TestPbTxtSource) {
  const string filename = StrCat(getenv("TEST_TMPDIR"), "/test_arch.pbtxt");
  WriteTextProtoOrDie(filename, ParseProtoFromStringOrDie<ArchitectureProto>(
                                    kTestArchitectureProto));
  EXPECT_THAT(*GetArchitectureProtoOrDie(StrCat(kPbTxtSource, ":", filename)),
              EqualsProto(kTestArchitectureProto));
}

TEST(ArchitectureProtoProviderTest, TestPbSource) {
  const string filename = StrCat(getenv("TEST_TMPDIR"), "/test_arch.pb");
  WriteBinaryProtoOrDie(filename, ParseProtoFromStringOrDie<ArchitectureProto>(
                                      kTestArchitectureProto));
  EXPECT_THAT(*GetArchitectureProtoOrDie(StrCat(kPbSource, ":", filename)),
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
                  StrCat(kRegisteredSource, ":test:provider:with:colon")),
              EqualsProto("name: 'an_id'"));
}

TEST(ArchitectureProtoProviderDeathTest, UnknownSource) {
  ASSERT_DEATH(GetArchitectureProtoOrDie("unknown_source"), "unknown_source");
}

TEST(ArchitectureProtoProviderDeathTest, UnknownProvider) {
  ASSERT_DEATH(
      GetArchitectureProtoOrDie(StrCat(kRegisteredSource, ":does_not_exist")),
      "test:provider");
}

}  // namespace
}  // namespace exegesis
