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

#include "llvm_sim/components/buffer.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/testing.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::Eq;
using testing::Pointee;

TEST(BufferTest, Works) {
  FifoBuffer<TestInputTag> Buffer(1000);

  {
    MockLogger Log;
    Buffer.Init(&Log);
  }
  CHECK_BUFFER_CONTENTS(Buffer, "[|]");
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty";
  ASSERT_TRUE(Buffer.Push(1));
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty before Propagate()";
  ASSERT_TRUE(Buffer.Push(2));
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty before Propagate()";
  CHECK_BUFFER_CONTENTS(Buffer, "[(2),(1)|]");

  {
    MockLogger Log1;
    testing::Sequence Seq1;
    EXPECT_CALL(Log1, Log(Eq("TestTag"), Eq("1"))).InSequence(Seq1);
    EXPECT_CALL(Log1, Log(Eq("TestTag"), Eq("2"))).InSequence(Seq1);
    Buffer.Propagate(&Log1);
  }
  CHECK_BUFFER_CONTENTS(Buffer, "[|(2),(1)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(1)));
  ASSERT_TRUE(Buffer.Push(3));
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(1)));
  CHECK_BUFFER_CONTENTS(Buffer, "[(3)|(2),(1)]");

  {
    MockLogger Log2;
    EXPECT_CALL(Log2, Log(Eq("TestTag"), Eq("3")));
    Buffer.Propagate(&Log2);
  }
  CHECK_BUFFER_CONTENTS(Buffer, "[|(3),(2),(1)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(1)));
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|(3),(2)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(2)));
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|(3)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(3)));
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|]");
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty:everything was popped";
  ASSERT_TRUE(Buffer.Push(4));
  CHECK_BUFFER_CONTENTS(Buffer, "[(4)|]");
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty before Propagate()";

  {
    MockLogger Log3;
    EXPECT_CALL(Log3, Log(Eq("TestTag"), Eq("4")));
    Buffer.Propagate(&Log3);
  }
  CHECK_BUFFER_CONTENTS(Buffer, "[|(4)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(4)));
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|]");
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty:everything was popped";
}

TEST(BufferTest, CapacityLimit) {
  FifoBuffer<TestInputTag> Buffer(2);

  MockLogger Log;
  testing::Sequence Seq;
  EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("1"))).InSequence(Seq);
  EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("2"))).InSequence(Seq);
  EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("5"))).InSequence(Seq);

  Buffer.Init(&Log);
  CHECK_BUFFER_CONTENTS(Buffer, "[|]");
  ASSERT_TRUE(Buffer.Push(1));
  ASSERT_TRUE(Buffer.Push(2));
  // Oops, full.
  ASSERT_FALSE(Buffer.Push(3));
  CHECK_BUFFER_CONTENTS(Buffer, "[(2),(1)|]");
  Buffer.Propagate(&Log);
  CHECK_BUFFER_CONTENTS(Buffer, "[|(2),(1)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(1)));
  // Still full.
  ASSERT_FALSE(Buffer.Push(4));
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|(2)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(2)));
  ASSERT_TRUE(Buffer.Push(5));
  CHECK_BUFFER_CONTENTS(Buffer, "[(5)|(2)]");
  Buffer.Propagate(&Log);
  CHECK_BUFFER_CONTENTS(Buffer, "[|(5),(2)]");
  Buffer.Pop();
  CHECK_BUFFER_CONTENTS(Buffer, "[|(5)]");
  ASSERT_THAT(Buffer.Peek(), Pointee(Eq(5)));
}

TEST(LinkBufferTest, Works) {
  LinkBuffer<TestInputTag> Link(2);

  {
    MockLogger Log;
    Link.Init(&Log);
  }
  ASSERT_EQ(Link.Peek(), nullptr) << "Link is empty";
  CHECK_BUFFER_CONTENTS(Link, "[|]");
  // Can push up to `Capacity` elements.
  ASSERT_TRUE(Link.Push(1));
  ASSERT_TRUE(Link.Push(2));
  ASSERT_FALSE(Link.Push(3));
  ASSERT_EQ(Link.Peek(), nullptr) << "Link is empty before Propagate()";
  CHECK_BUFFER_CONTENTS(Link, "[(2),(1)|]");

  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("1")));
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("2")));
    Link.Propagate(&Log);
  }
  CHECK_BUFFER_CONTENTS(Link, "[|(2),(1)]");
  ASSERT_THAT(Link.Peek(), Pointee(Eq(1)));
  // Can push for next cycle if staging is empty, even if there is an element in
  // the buffer.
  ASSERT_TRUE(Link.Push(3));
  ASSERT_TRUE(Link.Push(4));
  ASSERT_FALSE(Link.Push(5));
  CHECK_BUFFER_CONTENTS(Link, "[(4),(3)|(2),(1)]");
  ASSERT_THAT(Link.Peek(), Pointee(Eq(1)));

  Link.Pop();
  CHECK_BUFFER_CONTENTS(Link, "[(4),(3)|(2)]");
  ASSERT_THAT(Link.Peek(), Pointee(Eq(2)));
  // Propagate does nothing if all elements were not consumed (only one was).
  {
    MockLogger Log;
    Link.Propagate(&Log);
  }
  CHECK_BUFFER_CONTENTS(Link, "[(4),(3)|(2)]");
  ASSERT_THAT(Link.Peek(), Pointee(Eq(2)));

  // Consume the element, Propagate() should propagate.
  Link.Pop();
  CHECK_BUFFER_CONTENTS(Link, "[(4),(3)|]");
  ASSERT_EQ(Link.Peek(), nullptr) << "Link is empty: everything was popped";
  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("3")));
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("4")));
    Link.Propagate(&Log);
  }
  CHECK_BUFFER_CONTENTS(Link, "[|(4),(3)]");
  ASSERT_THAT(Link.Peek(), Pointee(Eq(3)));
}

TEST(LinkBufferTest, PushAgainBeforeAllConsumed) {
  LinkBuffer<TestInputTag> Link(3);

  {
    MockLogger Log;
    Link.Init(&Log);
  }
  CHECK_BUFFER_CONTENTS(Link, "[|]");
  ASSERT_TRUE(Link.Push(1));
  ASSERT_TRUE(Link.Push(2));
  CHECK_BUFFER_CONTENTS(Link, "[(2),(1)|]");
  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("1")));
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("2")));
    Link.Propagate(&Log);
  }
  CHECK_BUFFER_CONTENTS(Link, "[|(2),(1)]");
  Link.Pop();
  CHECK_BUFFER_CONTENTS(Link, "[|(2)]");

  // Push another element, propagate stalls because the previous elements have
  // not all been consumed.
  ASSERT_TRUE(Link.Push(3));
  CHECK_BUFFER_CONTENTS(Link, "[(3)|(2)]");
  {
    MockLogger Log;
    Link.Propagate(&Log);
    CHECK_BUFFER_CONTENTS(Link, "[(3)|(2)]");
  }

  // Push another element. This one fails as we're stalled.
  ASSERT_FALSE(Link.Push(4));
  CHECK_BUFFER_CONTENTS(Link, "[(3)|(2)]");

  // Still cannot propagate or push.
  {
    MockLogger Log;
    Link.Propagate(&Log);
  }
  ASSERT_FALSE(Link.Push(5));

  // Pop. We still cannot push (still stalled), but we can propagate (and
  // unstall).
  Link.Pop();
  CHECK_BUFFER_CONTENTS(Link, "[(3)|]");
  ASSERT_FALSE(Link.Push(4));
  CHECK_BUFFER_CONTENTS(Link, "[(3)|]");
  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("3")));
    Link.Propagate(&Log);
    CHECK_BUFFER_CONTENTS(Link, "[|(3)]");
  }
  // UNstalled, we can push.
  ASSERT_TRUE(Link.Push(6));
  CHECK_BUFFER_CONTENTS(Link, "[(6)|(3)]");
}

TEST(DevNullBufferTest, Works) {
  DevNullBuffer<TestInputTag> DevNull;

  {
    MockLogger Log;
    DevNull.Init(&Log);
  }
  // Can always push elements.
  ASSERT_TRUE(DevNull.Push(1));
  ASSERT_TRUE(DevNull.Push(2));

  // The buffer logs.
  {
    MockLogger Log;
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("1")));
    EXPECT_CALL(Log, Log(Eq("TestTag"), Eq("2")));
    DevNull.Propagate(&Log);
  }
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
