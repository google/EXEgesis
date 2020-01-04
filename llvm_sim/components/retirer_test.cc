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

#include "llvm_sim/components/retirer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/testing.h"
#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;
using testing::Field;

struct TestRetirerTag {
  struct Type {
    UopId::Type Uop;
  };
  static std::string Format(const Type& Elem) {
    return UopId::Format(Elem.Uop);
  }
  static const char kTagName[];
};
const char TestRetirerTag::kTagName[] = "TestRetirerTag";

TEST(IdentityComponentTest, Works) {
  Retirer<TestRetirerTag>::Config Config;

  llvm::MCInst Inst1;
  llvm::MCInst Inst2;
  Inst1.setOpcode(1);
  Inst2.setOpcode(2);
  const std::vector<llvm::MCInst> Instructions = {Inst1, Inst2};

  GlobalContext Context;
  // Setup fake decompositions. 2 uops for instr 0,
  {
    //  2 uops for instr 1.
    auto Decomposition = absl::make_unique<InstrUopDecomposition>();
    Decomposition->Uops.resize(2);
    Context.SetInstructionDecomposition(Inst1, std::move(Decomposition));
  }
  {
    //  1 uop for instr 2.
    auto Decomposition = absl::make_unique<InstrUopDecomposition>();
    Decomposition->Uops.resize(1);
    Context.SetInstructionDecomposition(Inst2, std::move(Decomposition));
  }

  TestSource<TestRetirerTag> Source;
  TestSink<TestRetirerTag> ElemSink;
  TestSink<InstructionIndex> InstructionSink;
  Retirer<TestRetirerTag> Retirer(&Context, Config, &Source, &ElemSink,
                                  &InstructionSink);
  Retirer.Init();

  const BlockContext BlockContext(Instructions, false);

  Source.Buffer_ = {
      {TestInstrIndex(0), 0}, {TestInstrIndex(0), 1}, {TestInstrIndex(1), 0}};
  Retirer.Tick(&BlockContext);
  ASSERT_THAT(Source.Buffer_, ElementsAre());
  ASSERT_THAT(ElemSink.Buffer_,
              ElementsAre(Field(&TestRetirerTag::Type::Uop, EqUopId(0, 0, 0)),
                          Field(&TestRetirerTag::Type::Uop, EqUopId(0, 0, 1)),
                          Field(&TestRetirerTag::Type::Uop, EqUopId(0, 1, 0))));
  ASSERT_THAT(InstructionSink.Buffer_,
              ElementsAre(EqInstrIndex(0, 0), EqInstrIndex(0, 1)));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
