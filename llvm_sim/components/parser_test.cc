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

#include "llvm_sim/components/parser.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;

TEST(ParserTest, MaxInstructionsPerCycleLimit) {
  InstructionParser::Config Config;
  Config.MaxInstructionsPerCycle = 2;

  GlobalContext Context;

  llvm::MCInst Inst;
  Inst.setOpcode(1);
  std::vector<llvm::MCInst> Instructions = {Inst, Inst, Inst, Inst};

  TestSource<InstructionIndex> Source;
  Source.Buffer_ = {TestInstrIndex(1), TestInstrIndex(2), TestInstrIndex(3)};
  TestSink<InstructionIndex> Sink;
  InstructionParser Parser(&Context, Config, &Source, &Sink);

  const BlockContext BlockContext(Instructions, false);
  Parser.Init();

  Parser.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(EqInstrIndex(0, 1), EqInstrIndex(0, 2)));

  // Simulate a full sink, no instructions should be parsed.
  Sink.Buffer_.clear();
  Sink.SetCapacity(0);
  Parser.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre());
  Sink.SetInfiniteCapacity();

  Sink.Buffer_.clear();
  Parser.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqInstrIndex(0, 3)));

  Sink.Buffer_.clear();
  Parser.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre());
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
