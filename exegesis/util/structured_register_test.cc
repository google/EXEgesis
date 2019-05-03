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

#include "exegesis/util/structured_register.h"

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(StructedRegisterTest, Unsigned32Bit) {
  StructuredRegister<uint32_t> reg(0xF0E0D0C0);
  EXPECT_EQ((reg.ValueAt<31, 24>()), 0xF0);
  EXPECT_EQ((reg.ValueAt<7, 0>()), 0xC0);

  *reg.mutable_raw_value() = 0x0A0B0C0D;
  EXPECT_EQ((reg.ValueAt<23, 16>()), 0x0B);
  EXPECT_EQ((reg.ValueAt<15, 8>()), 0x0C);
}

TEST(StructuredRegisterTest, Unsigned64Bit) {
  StructuredRegister<uint64_t> reg(0xFEDCBA9876543210);
  EXPECT_EQ((reg.ValueAt<63, 56>()), 0xFE);
  EXPECT_EQ((reg.ValueAt<11, 4>()), 0x21);
}

}  // namespace
}  // namespace exegesis
