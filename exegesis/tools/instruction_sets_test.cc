#include "exegesis/tools/instruction_sets.h"

#include "base/macros.h"
#include "exegesis/base/transform_factory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

DECLARE_string(exegesis_architecture);

using testing::EqualsProto;
using testing::Not;

namespace exegesis {
namespace {

// Make sure that the default value of the command-line flag
// --exegesis_architecture always resolves to an existing instruction
// set.
TEST(InstructionSetsTest, GetInstructionSetFromCommandLineFlags) {
  const InstructionSetProto instruction_set =
      GetTransformedInstructionSetFromCommandLineFlags();
  ASSERT_GT(instruction_set.instructions_size(), 0);
}

TEST(InstructionSetsTest, DefaultTransform) {
  FLAGS_exegesis_architecture = "registered:intel";
  FLAGS_exegesis_transforms = "";
  const InstructionSetProto raw =
      GetTransformedInstructionSetFromCommandLineFlags();

  FLAGS_exegesis_architecture = "registered:intel";
  FLAGS_exegesis_transforms =
      "RemoveSpecialCaseInstructions,RemoveImplicitXmm0Operand";
  const InstructionSetProto raw_with_custom =
      GetTransformedInstructionSetFromCommandLineFlags();
  EXPECT_THAT(raw, Not(EqualsProto(raw_with_custom)));

  FLAGS_exegesis_architecture = "registered:intel";
  FLAGS_exegesis_transforms = "default";
  const InstructionSetProto raw_with_default =
      GetTransformedInstructionSetFromCommandLineFlags();
  EXPECT_THAT(raw, Not(EqualsProto(raw_with_default)));
  EXPECT_THAT(raw_with_custom, Not(EqualsProto(raw_with_default)));
}

}  // namespace
}  // namespace exegesis
