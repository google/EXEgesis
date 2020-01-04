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

#include "llvm_sim/framework/simulator.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Property;
using ::testing::WithArg;

class TestBuffer : public Buffer {
 public:
  MOCK_METHOD1(Init, void(Logger*));
  MOCK_METHOD1(Propagate, void(Logger*));
};

class TestComponent : public Component {
 public:
  using Component::Component;
  MOCK_METHOD0(Init, void());
  MOCK_METHOD1(Tick, void(const BlockContext* BlockContext));
};

template <char C>
void LogChar(Logger* Log) {
  Log->Log("TestTag", std::string(1, C));
}

// Log line matcher
MATCHER_P4(EqLine, BufferIndex, Cycle, MsgTag, Msg, "") {
  if (arg.BufferIndex == BufferIndex && arg.Cycle == Cycle &&
      arg.MsgTag == MsgTag && arg.Msg == Msg) {
    return true;
  }
  *result_listener << "(buffer=" << arg.BufferIndex << ", cycle=" << arg.Cycle
                   << ", msg_tag=\"" << arg.MsgTag << "\", msg=\"" << arg.Msg
                   << "\")";
  return false;
}

TEST(SimulatorTest, Works) {
  const GlobalContext Context;
  auto Component1 = absl::make_unique<TestComponent>(&Context);
  auto Component2 = absl::make_unique<TestComponent>(&Context);
  auto Buffer1 = absl::make_unique<TestBuffer>();
  auto Buffer2 = absl::make_unique<TestBuffer>();

  testing::Sequence Seq;
  EXPECT_CALL(*Component1, Init()).InSequence(Seq);
  EXPECT_CALL(*Component2, Init()).InSequence(Seq);
  EXPECT_CALL(*Buffer1, Init(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Buffer2, Init(NotNull())).InSequence(Seq);

  EXPECT_CALL(*Component1, Tick(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Component2, Tick(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Buffer1, Propagate(NotNull()))
      .InSequence(Seq)
      .WillOnce(WithArg<0>(Invoke(&LogChar<'A'>)));
  EXPECT_CALL(*Buffer2, Propagate(NotNull()))
      .InSequence(Seq)
      .WillOnce(WithArg<0>(Invoke(&LogChar<'B'>)));
  EXPECT_CALL(*Component1, Tick(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Component2, Tick(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Buffer1, Propagate(NotNull())).InSequence(Seq);
  EXPECT_CALL(*Buffer2, Propagate(NotNull()))
      .InSequence(Seq)
      .WillOnce(WithArg<0>(Invoke(&LogChar<'D'>)));

  Simulator Simulator;
  Simulator.AddComponent(std::move(Component1));
  Simulator.AddComponent(std::move(Component2));
  BufferDescription BufferDescription1;
  BufferDescription1.DisplayName = "BD1";
  Simulator.AddBuffer(std::move(Buffer1), BufferDescription1);
  BufferDescription BufferDescription2;
  BufferDescription2.DisplayName = "BD2";
  Simulator.AddBuffer(std::move(Buffer2), BufferDescription2);
  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  constexpr int kMaxNumCycles = 2;
  const auto Result = Simulator.Run(BlockContext, 0, kMaxNumCycles);
  EXPECT_THAT(Result->NumCycles, kMaxNumCycles);
  EXPECT_THAT(Result->Lines, ElementsAre(EqLine(0, 0, "TestTag", "A"),
                                         EqLine(1, 0, "TestTag", "B"),
                                         EqLine(1, 1, "TestTag", "D")));
  EXPECT_THAT(Result->BufferDescriptions,
              ElementsAre(Field(&BufferDescription::DisplayName, Eq("BD1")),
                          Field(&BufferDescription::DisplayName, Eq("BD2"))));
}

TEST(SimulatorTest, Iterations) {
  const GlobalContext Context;
  std::vector<llvm::MCInst> Instructions(2);
  const BlockContext BlockContext(Instructions, true);

  Simulator Simulator;
  auto Component1 = absl::make_unique<TestComponent>(&Context);

  // A gmock action that pushes the given instruction to the simulator's sink.
  const auto SetInstructionDone = [&Simulator](size_t Iter, size_t BBIndex) {
    return testing::InvokeWithoutArgs([&Simulator, Iter, BBIndex]() {
      EXPECT_TRUE(Simulator.GetInstructionSink()->Push({BBIndex, Iter}));
    });
  };
  testing::Sequence Seq;
  EXPECT_CALL(*Component1, Init()).InSequence(Seq);
  // Cycle 0 does nothing.
  EXPECT_CALL(*Component1, Tick(NotNull())).InSequence(Seq);
  // Cycle 1 executes two instructions (one complete iteration).
  EXPECT_CALL(*Component1, Tick(NotNull()))
      .InSequence(Seq)
      .WillOnce(DoAll(SetInstructionDone(0, 0), SetInstructionDone(0, 1)));
  // Cycle 2 executes one instruction (half of the second iteration).
  EXPECT_CALL(*Component1, Tick(NotNull()))
      .InSequence(Seq)
      .WillOnce(SetInstructionDone(1, 0));
  // Cycle 3 executes one instructions (finishes the second iteration).
  EXPECT_CALL(*Component1, Tick(NotNull()))
      .InSequence(Seq)
      .WillOnce(SetInstructionDone(1, 1));

  Simulator.AddComponent(std::move(Component1));

  const auto Result =
      Simulator.Run(BlockContext, /*2 iterations*/ 2, /*unlimited cycles*/ 0);

  EXPECT_THAT(Result->NumCycles, 4);
  for (const auto& Line : Result->Lines) {
    EXPECT_LT(Line.Cycle, Result->NumCycles);
  }
  EXPECT_THAT(
      Result->Iterations,
      ElementsAre(Field(&SimulationLog::IterationStats::EndCycle, Eq(1)),
                  Field(&SimulationLog::IterationStats::EndCycle, Eq(3))));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
