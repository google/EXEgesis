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

#include "llvm_sim/components/fetcher.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;
using testing::Eq;
using testing::IsNull;
using testing::Pointee;

class TestMCCodeEmitter : public llvm::MCCodeEmitter {
 public:
  ~TestMCCodeEmitter() override {}

  void encodeInstruction(const llvm::MCInst &Inst, llvm::raw_ostream &OS,
                         llvm::SmallVectorImpl<llvm::MCFixup> &Fixups,
                         const llvm::MCSubtargetInfo &STI) const override {
    switch (Inst.getOpcode()) {
      case 2:
        OS << "abcd";
        return;
      case 3:
        OS << "a";
        return;
      default:
        FAIL() << "unknown opcode " << Inst.getOpcode();
        return;
    }
  }
};

class FetcherTest : public ::testing::Test {
 protected:
  FetcherTest() {
    // One 4-byte fixed-length and one 1-byte variable-length instruction.
    Inst4ByteFixed_.setOpcode(1);
    Inst4ByteVariable_.setOpcode(2);
    Inst1ByteVariable_.setOpcode(3);
    InstrDesc_[1].Size = 4;
    InstrDesc_[2].Size = 0;  // Variable size.
    InstrDesc_[3].Size = 0;  // Variable size.
    auto InstrInfo = absl::make_unique<llvm::MCInstrInfo>();
    InstrInfo->InitMCInstrInfo(InstrDesc_.data(), nullptr, nullptr,
                               InstrDesc_.size());
    Context_.InstrInfo = std::move(InstrInfo);
    Context_.CodeEmitter = absl::make_unique<TestMCCodeEmitter>();
  }

  llvm::MCInst Inst4ByteFixed_;
  llvm::MCInst Inst4ByteVariable_;
  llvm::MCInst Inst1ByteVariable_;
  std::array<llvm::MCInstrDesc, 4> InstrDesc_;
  GlobalContext Context_;
};

TEST_F(FetcherTest, BytesPerCycleLimit) {
  Fetcher::Config Config;
  Config.MaxBytesPerCycle = 9;

  TestSink<InstructionIndex> Sink;
  Fetcher Fetcher(&Context_, Config, &Sink);
  std::vector<llvm::MCInst> Instructions = {Inst4ByteVariable_, Inst4ByteFixed_,
                                            Inst4ByteFixed_};
  const BlockContext BlockContext(Instructions, false);
  Fetcher.Init();

  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(EqInstrIndex(0, 0), EqInstrIndex(0, 1)));

  Sink.Buffer_.clear();
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqInstrIndex(0, 2)));

  Sink.Buffer_.clear();
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre());
}

TEST_F(FetcherTest, BytesPerCycleLimitSmall) {
  Fetcher::Config Config;
  Config.MaxBytesPerCycle = 6;

  TestSink<InstructionIndex> Sink;
  Fetcher Fetcher(&Context_, Config, &Sink);
  std::vector<llvm::MCInst> Instructions = {
      Inst4ByteVariable_, Inst1ByteVariable_, Inst1ByteVariable_};
  const BlockContext BlockContext(Instructions, false);
  Fetcher.Init();

  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqInstrIndex(0, 0), EqInstrIndex(0, 1),
                                        EqInstrIndex(0, 2)));
}

TEST_F(FetcherTest, LoopContext) {
  Fetcher::Config Config;
  Config.MaxBytesPerCycle = 9;

  TestSink<InstructionIndex> Sink;
  Fetcher Fetcher(&Context_, Config, &Sink);
  std::vector<llvm::MCInst> Instructions = {Inst4ByteFixed_, Inst4ByteFixed_,
                                            Inst4ByteFixed_};
  const BlockContext BlockContext(Instructions, true);
  Fetcher.Init();

  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(EqInstrIndex(0, 0), EqInstrIndex(0, 1)));

  Sink.Buffer_.clear();
  Fetcher.Tick(&BlockContext);
  // Note that basic block wrapping has to wait for the next cycle.
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqInstrIndex(0, 2)));

  Sink.Buffer_.clear();
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(EqInstrIndex(1, 0), EqInstrIndex(1, 1)));

  Sink.Buffer_.clear();
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre(EqInstrIndex(1, 2)));
}

TEST_F(FetcherTest, FullSink) {
  Fetcher::Config Config;
  Config.MaxBytesPerCycle = 9;

  TestSink<InstructionIndex> Sink;
  Fetcher Fetcher(&Context_, Config, &Sink);
  std::vector<llvm::MCInst> Instructions = {Inst4ByteFixed_, Inst4ByteFixed_,
                                            Inst4ByteFixed_};
  const BlockContext BlockContext(Instructions, false);
  Fetcher.Init();

  // Simulate a full sink, no instructions should be fetched.
  Sink.SetCapacity(0);
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_, ElementsAre());

  Sink.Buffer_.clear();
  Sink.SetInfiniteCapacity();
  Fetcher.Tick(&BlockContext);
  ASSERT_THAT(Sink.Buffer_,
              ElementsAre(EqInstrIndex(0, 0), EqInstrIndex(0, 1)));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
