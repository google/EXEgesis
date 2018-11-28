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

#include "exegesis/x86/registers.h"

#include <algorithm>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::testing::Not;

RegisterGroupProto FindGroupByRegister(const RegisterSetProto& registers,
                                       const std::string& register_name) {
  for (const RegisterGroupProto& group : registers.register_groups()) {
    for (const RegisterProto& reg : group.registers()) {
      if (reg.name() == register_name) {
        return group;
      }
    }
  }
  return RegisterGroupProto();
}

TEST(GetRegisterSetTest, IsNotEmpty) {
  const RegisterSetProto& registers = GetRegisterSet();
  EXPECT_GT(registers.register_groups_size(), 0);

  for (const RegisterGroupProto& group : registers.register_groups()) {
    SCOPED_TRACE(absl::StrCat("group.name = ", group.name()));
    EXPECT_GT(group.registers_size(), 0);

    for (const RegisterProto& reg : group.registers()) {
      EXPECT_FALSE(reg.name().empty());
    }
  }
}

TEST(GetRegisterSetTest, RegisterNamesAreUnique) {
  const RegisterSetProto& registers = GetRegisterSet();
  absl::flat_hash_set<std::string> register_names;
  for (const RegisterGroupProto& group : registers.register_groups()) {
    for (const RegisterProto& reg : group.registers()) {
      SCOPED_TRACE(absl::StrCat("reg.name = ", reg.name()));
      EXPECT_TRUE(register_names.insert(reg.name()).second);
    }
  }
}

TEST(GetRegisterSetTest, PositionsAreValid) {
  const RegisterSetProto& registers = GetRegisterSet();
  // Check that all registers have a position in the group, that at all bit
  // indices are non-negative and at least one register starts at bit zero.
  for (const RegisterGroupProto& group : registers.register_groups()) {
    SCOPED_TRACE(absl::StrCat("group.name = ", group.name()));
    bool has_register_starting_at_zero;
    for (const RegisterProto& reg : group.registers()) {
      SCOPED_TRACE(absl::StrCat("reg.name = ", reg.name()));
      EXPECT_TRUE(reg.has_position_in_group());
      const BitRange& position = reg.position_in_group();
      EXPECT_GE(position.lsb(), 0);
      EXPECT_GT(position.msb(), position.lsb());
      if (position.lsb() == 0) has_register_starting_at_zero = true;
    }
    EXPECT_TRUE(has_register_starting_at_zero);
  }
}

TEST(GetRegisterSetTest, PositionsAreOverlapping) {
  const RegisterSetProto& registers = GetRegisterSet();
  for (const RegisterGroupProto& group : registers.register_groups()) {
    SCOPED_TRACE(absl::StrCat("group.name = ", group.name()));
    std::vector<const BitRange*> ranges_by_lsb(group.registers_size(), nullptr);
    std::transform(group.registers().begin(), group.registers().end(),
                   ranges_by_lsb.begin(), [](const RegisterProto& reg) {
                     return &reg.position_in_group();
                   });
    std::sort(ranges_by_lsb.begin(), ranges_by_lsb.end(),
              [](const BitRange* a, const BitRange* b) {
                CHECK(a != nullptr);
                CHECK(b != nullptr);
                return a->lsb() < b->lsb();
              });
    // Check that there are no non-overlapping groups. If they were, the most
    // significant bit used in the "lower" group would come strictly before the
    // least significant bit of the "higher" group. We check the bit ranges in
    // such order that the least significant bit is non-decreasing. To prove
    // that there is no split into two non-overlapping groups, it is sufficient
    // to check that the index of the least significant bit of the current group
    // is lower or equal to the index of the most significant bit of at least
    // one preceding group.
    int max_bit = 0;
    for (const BitRange* range : ranges_by_lsb) {
      EXPECT_LE(range->lsb(), max_bit);
      max_bit = std::max(max_bit, range->msb());
    }
  }
}

TEST(GetRegisterSetTest, CheckSomeRegisters) {
  constexpr struct {
    const char* register_name;
    const char* expected_proto;
  } kTestCases[] =  //
      {{"RAX", R"proto(
          name: 'RAX group'
          description: 'The group of registers aliased with RAX'
          registers {
            name: 'RAX'
            binary_encoding: 0
            register_class: GENERAL_PURPOSE_REGISTER_64_BIT
            position_in_group { msb: 63 }
          }
          registers {
            name: 'EAX'
            binary_encoding: 0
            register_class: GENERAL_PURPOSE_REGISTER_32_BIT
            position_in_group { msb: 31 }
          }
          registers {
            name: 'AX'
            binary_encoding: 0
            register_class: GENERAL_PURPOSE_REGISTER_16_BIT
            position_in_group { msb: 15 }
          }
          registers {
            name: 'AL'
            binary_encoding: 0
            register_class: GENERAL_PURPOSE_REGISTER_8_BIT
            position_in_group { msb: 7 }
          }
          registers {
            name: 'AH'
            binary_encoding: 4
            register_class: GENERAL_PURPOSE_REGISTER_8_BIT
            position_in_group { lsb: 8 msb: 15 }
          })proto"},
       {"R16", ""},
       {"ST7", R"proto(
          name: 'ST7 group'
          description: 'The group of registers aliased with ST7'
          registers {
            name: 'ST7'
            binary_encoding: 7
            register_class: FLOATING_POINT_STACK_REGISTER
            position_in_group { msb: 79 }
            feature_name: 'FPU'
          }
          registers {
            name: 'MM7'
            binary_encoding: 7
            register_class: MMX_STACK_REGISTER
            position_in_group { msb: 63 }
            feature_name: 'MMX'
          })proto"},
       {"DS", R"proto(
          name: 'DS group'
          description: 'The group of registers aliased with DS'
          registers {
            name: 'DS'
            binary_encoding: 3
            register_class: SPECIAL_REGISTER_SEGMENT
            position_in_group { lsb: 0 msb: 15 }
          })proto"},
       {"XMM15", R"proto(
          name: 'XMM15 group'
          description: 'The group of registers aliased with XMM15'
          registers {
            name: 'XMM15'
            binary_encoding: 15
            register_class: VECTOR_REGISTER_128_BIT
            position_in_group { msb: 127 }
            feature_name: 'SSE'
          }
          registers {
            name: 'YMM15'
            binary_encoding: 15
            register_class: VECTOR_REGISTER_256_BIT
            position_in_group { msb: 255 }
            feature_name: 'AVX'
          }
          registers {
            name: 'ZMM15'
            binary_encoding: 15
            register_class: VECTOR_REGISTER_512_BIT
            position_in_group { msb: 511 }
            feature_name: 'AVX512'
          })proto"},
       {"XMM17", R"proto(
          name: 'XMM17 group'
          description: 'The group of registers aliased with XMM17'
          registers {
            name: 'XMM17'
            binary_encoding: 17
            register_class: VECTOR_REGISTER_128_BIT
            position_in_group { msb: 127 }
            feature_name: 'AVX512'
          }
          registers {
            name: 'YMM17'
            binary_encoding: 17
            register_class: VECTOR_REGISTER_256_BIT
            position_in_group { msb: 255 }
            feature_name: 'AVX512'
          }
          registers {
            name: 'ZMM17'
            binary_encoding: 17
            register_class: VECTOR_REGISTER_512_BIT
            position_in_group { msb: 511 }
            feature_name: 'AVX512'
          })proto"},
       {"CR3", R"proto(
          name: 'CR3 group'
          description: 'The group of registers aliased with CR3'
          registers {
            name: 'CR3'
            binary_encoding: 3
            register_class: SPECIAL_REGISTER_CONTROL
            position_in_group { msb: 63 }
          })proto"},
       {"DR3", R"proto(
          name: 'DR3 group'
          description: 'The group of registers aliased with DR3'
          registers {
            name: 'DR3'
            binary_encoding: 3
            register_class: SPECIAL_REGISTER_DEBUG
            position_in_group { msb: 63 }
          })proto"},
       {"BND1", R"proto(
          name: "BND1 group"
          description: "The group of registers aliased with BND1"
          registers {
            name: "BND1"
            binary_encoding: 1
            position_in_group { msb: 127 }
            feature_name: "MPX"
            register_class: SPECIAL_REGISTER_MPX_BOUNDS
          })proto"}};
  for (const auto& test_case : kTestCases) {
    const std::string register_name = test_case.register_name;
    SCOPED_TRACE(absl::StrCat("register_name = ", register_name));
    EXPECT_THAT(FindGroupByRegister(GetRegisterSet(), test_case.register_name),
                EqualsProto(test_case.expected_proto));
  }
  // The definitions of the following registers are too long to repeat here. We
  // only check that we have some definition for them.
  constexpr const char* kCheckedRegisters[] = {
      "RFLAGS", "EFLAGS", "FPSW", "MXCSR",     "FPCW",    "GDTR",
      "LDTR",   "IDTR",   "TR",   "BNDSTATUS", "BNDCFGS", "BNDCFGU"};
  for (const std::string register_name : kCheckedRegisters) {
    SCOPED_TRACE(absl::StrCat("register_name = ", register_name));
    EXPECT_THAT(FindGroupByRegister(GetRegisterSet(), register_name),
                Not(EqualsProto("")));
  }
}

}  // namespace
}  // namespace x86
}  // namespace exegesis
