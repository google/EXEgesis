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

#include "llvm_sim/components/simplified_execution_units.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::AnyOf;
using testing::Eq;
using testing::Pointee;
using testing::UnorderedElementsAre;

TEST(SimplifiedExecutionUnitsTest, Works) {
  const SimplifiedExecutionUnits<TestExecutionUnitInputTag>::Config Config;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  TestSink<TestExecutionUnitInputTag> Sink;
  SimplifiedExecutionUnits<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                           &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  Source.Buffer_ = {{0, /*Latency*/ 1}, {1, /*Latency*/ 2}, {2, /*Latency*/ 3}};
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, UnorderedElementsAre());
  ASSERT_THAT(Sink.Buffer_, UnorderedElementsAre(HasId(0)));

  Sink.Buffer_.clear();
  Source.Buffer_ = {{3, /*Latency*/ 1}, {4, /*Latency*/ 2}};
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, UnorderedElementsAre());
  ASSERT_THAT(Sink.Buffer_, UnorderedElementsAre(HasId(1), HasId(3)));

  Sink.Buffer_.clear();
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, UnorderedElementsAre());
  ASSERT_THAT(Sink.Buffer_, UnorderedElementsAre(HasId(2), HasId(4)));

  Sink.Buffer_.clear();
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, UnorderedElementsAre());
}

TEST(ExecDepsBufferTest, Works) {
  ExecDepsBuffer<TestExecutionUnitInputTag> Buffer;

  {
    MockLogger Log;
    Buffer.Init(&Log);
  }
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty";
  ASSERT_TRUE(Buffer.Push({0, /*Latency*/ 1}));
  ASSERT_TRUE(Buffer.Push({1, /*Latency*/ 2}));
  ASSERT_TRUE(Buffer.Push({2, /*Latency*/ 1}));
  ASSERT_EQ(Buffer.Peek(), nullptr) << "Buffer is empty before Propagate()";

  {
    MockLogger Log1;
    Buffer.Propagate(&Log1);
  }
  ASSERT_THAT(Buffer.Peek(), Pointee(AnyOf(HasId(0), HasId(2))));
  Buffer.Pop();
  ASSERT_THAT(Buffer.Peek(), Pointee(AnyOf(HasId(0), HasId(2))));
  Buffer.Pop();
  ASSERT_EQ(Buffer.Peek(), nullptr);

  {
    MockLogger Log1;
    EXPECT_CALL(Log1, Log(Eq("TestExecutionUnitInputTag"), Eq("1")));
    Buffer.Propagate(&Log1);
  }
  ASSERT_THAT(Buffer.Peek(), Pointee(HasId(1)));
  Buffer.Pop();
  ASSERT_EQ(Buffer.Peek(), nullptr);
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
