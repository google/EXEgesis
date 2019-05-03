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

#include "llvm_sim/components/execution_unit.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;

TEST(ExecutionUnitTest, TwoStageUnpipelined) {
  NonPipelinedExecutionUnit<TestExecutionUnitInputTag>::Config Config;
  Config.NumStages = 2;
  constexpr const int kLatency = 2;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  Source.Buffer_ = {{1, kLatency}, {2, kLatency}};
  TestSink<TestExecutionUnitInputTag> Sink;
  NonPipelinedExecutionUnit<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                            &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  // The unit grabs the first element from the source.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the second stage and gets written back.
  // The unit grabs the second element.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it though the second stage and gets written back.
  // The unit has nothing to grab.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));

  // The unit has nothing to execute.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));

  // The unit has nothing to execute.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));
}

TEST(ExecutionUnitTest, TwoStagePipelined) {
  PipelinedExecutionUnit<TestExecutionUnitInputTag>::Config Config;
  Config.NumStages = 2;
  Config.NumCyclesPerStage = 1;
  constexpr const int kLatency = 2;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  Source.Buffer_ = {{1, kLatency}, {2, kLatency}};
  TestSink<TestExecutionUnitInputTag> Sink;
  PipelinedExecutionUnit<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                         &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  // The unit grabs the first element from the source.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the first stage.
  // The unit grabs the second element.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the second stage and gets written back.
  // The second element makes it though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it though the second stage and gets written back.
  // The unit has nothing to grab.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));
}

TEST(ExecutionUnitTest, TwoStagePipelinedTwoCyclesPerStage) {
  PipelinedExecutionUnit<TestExecutionUnitInputTag>::Config Config;
  Config.NumStages = 2;
  Config.NumCyclesPerStage = 2;
  constexpr const int kLatency = 4;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  Source.Buffer_ = {{1, kLatency}, {2, kLatency}};
  TestSink<TestExecutionUnitInputTag> Sink;
  PipelinedExecutionUnit<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                         &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  // The unit grabs the first element from the source.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it halfway though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the first stage.
  // The unit grabs the second element.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it halfway though the second stage.
  // The second element makes it halfway though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the second stage and gets written back.
  // The second element makes it though the first stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it halfway though the second stage.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it though the second stage and gets written back.
  // The unit has nothing to grab.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));
}

TEST(ExecutionUnitTest, TwoStagePipelinedWithStall) {
  PipelinedExecutionUnit<TestExecutionUnitInputTag>::Config Config;
  Config.NumStages = 2;
  Config.NumCyclesPerStage = 1;
  constexpr const int kLatency = 2;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  Source.Buffer_ = {{1, kLatency}, {2, kLatency}};
  TestSink<TestExecutionUnitInputTag> Sink;
  PipelinedExecutionUnit<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                         &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  // The unit grabs the first element from the source.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(2)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the first stage.
  // The unit grabs the second element.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // The first element makes it though the second stage. But the sink is full,
  // so everything stalls.
  Sink.SetCapacity(0);
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // Unstall: The first element gets written back.
  // The second element makes it though the first stage.
  Sink.SetInfiniteCapacity();
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1)));

  // The second element makes it though the second stage and gets written back.
  // The unit has nothing to grab.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(Sink.Buffer_, ElementsAre(HasId(1), HasId(2)));
}

TEST(ExecutionUnitTest, OnlyExecutesOnesElements) {
  PipelinedExecutionUnit<TestExecutionUnitInputTag>::Config Config;
  Config.NumStages = 2;
  Config.NumCyclesPerStage = 1;
  constexpr const unsigned kLatency = 3;

  GlobalContext Context;

  TestSource<TestExecutionUnitInputTag> Source;
  Source.Buffer_ = {{1, kLatency}};
  TestSink<TestExecutionUnitInputTag> Sink;
  PipelinedExecutionUnit<TestExecutionUnitInputTag> Unit(&Context, Config,
                                                         &Source, &Sink);

  std::vector<llvm::MCInst> Instructions;
  const BlockContext BlockContext(Instructions, false);
  Unit.Init();

  // The unit does not grab anything, the elements are not for her.
  Unit.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre(HasId(1)));
  ASSERT_THAT(Sink.Buffer_, ElementsAre());
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
