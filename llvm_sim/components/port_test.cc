// Copyright 2018 Google Inc.
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

#include "llvm_sim/components/port.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::Eq;
using testing::Pointee;

TEST(PortTest, Works) {
  IssuePort<TestInputTag> Port;

  {
    MockLogger Log;
    Port.Init(&Log);
  }
  ASSERT_EQ(Port.Peek(), nullptr) << "Port is empty";
  // Can push if there is nothing at all in the buffer.
  ASSERT_TRUE(Port.Push(1));
  ASSERT_EQ(Port.Peek(), nullptr) << "Port is empty before Propagate()";
  // Cannot push twice during one cycle.
  ASSERT_FALSE(Port.Push(2));
  ASSERT_EQ(Port.Peek(), nullptr) << "Port is empty before Propagate()";

  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("1")));
    Port.Propagate(&Log);
  }
  ASSERT_THAT(Port.Peek(), Pointee(Eq(1)));
  // Can push for next cycle if staging is empty, even if there is an element in
  // the buffer.
  ASSERT_TRUE(Port.Push(3));
  ASSERT_THAT(Port.Peek(), Pointee(Eq(1)));
  // Still cannot push twice during one cycle.
  ASSERT_FALSE(Port.Push(4));

  // Propagate does nothing if the element was not consumed.
  {
    MockLogger Log;
    Port.Propagate(&Log);
  }
  ASSERT_THAT(Port.Peek(), Pointee(Eq(1)));

  // Consume the element, Propagate() should propagate.
  Port.Pop();
  ASSERT_EQ(Port.Peek(), nullptr) << "Port is empty:everything was popped";
  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("3")));
    Port.Propagate(&Log);
  }
  ASSERT_THAT(Port.Peek(), Pointee(Eq(3)));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
