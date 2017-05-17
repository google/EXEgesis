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

#include "exegesis/base/port_mask.h"

#include <cstdint>

#include "exegesis/proto/microarchitecture.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(PortMaskTest, Comparison) {
  PortMask p;
  p.AddPossiblePort(0);
  p.AddPossiblePort(1);
  p.AddPossiblePort(5);
  p.AddPossiblePort(6);
  const uint64_t kMask = (1 << 0) + (1 << 1) + (1 << 5) + (1 << 6);
  EXPECT_EQ(p, PortMask(kMask));
  EXPECT_NE(p, PortMask(kMask - (1 << 6)));
}

TEST(PortMaskTest, ToString) {
  PortMask p;
  p.AddPossiblePort(0);
  p.AddPossiblePort(1);
  p.AddPossiblePort(5);
  p.AddPossiblePort(6);
  EXPECT_EQ(p.ToString(), "P0156");
}

TEST(PortMaskTest, InitFromProto) {
  PortMaskProto proto;
  proto.add_port_numbers(0);
  proto.add_port_numbers(1);
  proto.add_port_numbers(5);
  PortMask p(proto);
  EXPECT_EQ(p.ToString(), "P015");
}

TEST(PortMaskTest, ConvertToPooto) {
  PortMask p;
  p.AddPossiblePort(0);
  p.AddPossiblePort(1);
  p.AddPossiblePort(5);
  p.AddPossiblePort(6);
  PortMaskProto proto = p.ToProto();
  PortMask q(proto);
  EXPECT_EQ(q.ToString(), "P0156");
}

TEST(PortMaskTest, InitFromString) {
  PortMask p("P01p56");
  EXPECT_EQ(p.ToString(), "P0156");
}

TEST(PortMaskTest, EmptyThenAdd) {
  PortMask p;
  EXPECT_EQ(p.ToString(), "");
  p.AddPossiblePort(0);
  EXPECT_EQ(p.ToString(), "P0");
}

TEST(PortMaskTest, Iterator) {
  {
    // Checking end and past-the-end.
    PortMask::const_iterator end;
    EXPECT_EQ(end, PortMask::const_iterator());
    ++end;
    EXPECT_EQ(end, PortMask::const_iterator());
  }

  PortMask p("p156");
  const std::vector<int> ports(p.begin(), p.end());
  EXPECT_THAT(ports, testing::ElementsAre(1, 5, 6));
}

}  // anonymous namespace
}  // namespace exegesis
