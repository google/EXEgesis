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

#include "llvm_sim/components/decoder.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;

class DecoderTest : public ::testing::Test {
 protected:
  DecoderTest() {
    // Create a basic block with three instructions.
    llvm::MCInst Inst1;
    llvm::MCInst Inst2;
    Inst1.setOpcode(1);
    Inst2.setOpcode(2);
    Instructions_ = {Inst1, Inst2, Inst1};

    // Setup fake decompositions.
    {
      auto Decomposition1 = absl::make_unique<InstrUopDecomposition>();
      Decomposition1->Uops.resize(2);
      Context_.SetInstructionDecomposition(Inst1, std::move(Decomposition1));
    }
    {
      auto Decomposition2 = absl::make_unique<InstrUopDecomposition>();
      Decomposition2->Uops.resize(1);
      Context_.SetInstructionDecomposition(Inst2, std::move(Decomposition2));
    }
  }

  std::vector<llvm::MCInst> Instructions_;
  GlobalContext Context_;
};

TEST_F(DecoderTest, Works) {
  InstructionDecoder::Config Config;
  Config.NumDecoders = 2;

  TestSource<InstructionIndex> Source;
  TestSink<UopId> Sink;
  Source.Buffer_ = {TestInstrIndex(0), TestInstrIndex(1), TestInstrIndex(2)};
  InstructionDecoder Decoder(&Context_, Config, &Source, &Sink);
  const BlockContext BlockContext(Instructions_, false);
  Decoder.Init();

  Decoder.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqUopId(0, 0, 0), EqUopId(0, 0, 1),
                                        EqUopId(0, 1, 0)));

  // Simulate a sink that can accept only a single uop. Since there are two
  // uops in the third instructions, decoding should stall.
  Sink.Buffer_.clear();
  Sink.SetCapacity(1);
  Decoder.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  // This time there is enough capacity in the sink.
  Sink.Buffer_.clear();
  Sink.SetCapacity(2);
  Decoder.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqUopId(0, 2, 0), EqUopId(0, 2, 1)));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
