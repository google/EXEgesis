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

#include "llvm_sim/framework/component.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace simulator {
namespace {

TEST(ComponentTest, InstructionIndexFormat) {
  InstructionIndex::Type In;
  In.Iteration = 1789;
  In.BBIndex = 42;
  const std::string Formatted = InstructionIndex::Format(In) + "abcd";
  llvm::StringRef Input = Formatted;
  InstructionIndex::Type Out;
  ASSERT_FALSE(InstructionIndex::Consume(Input, Out));
  ASSERT_EQ(Out.Iteration, In.Iteration);
  ASSERT_EQ(Out.BBIndex, In.BBIndex);
  ASSERT_EQ(Input, "abcd");
}

TEST(ComponentTest, InstructionIndexFormatFail) {
  llvm::StringRef Input;
  InstructionIndex::Type Out;

  Input = "abc,10";
  ASSERT_TRUE(InstructionIndex::Consume(Input, Out));

  Input = "10,abc";
  ASSERT_TRUE(InstructionIndex::Consume(Input, Out));

  Input = "10";
  ASSERT_TRUE(InstructionIndex::Consume(Input, Out));

  Input = "10;12";
  ASSERT_TRUE(InstructionIndex::Consume(Input, Out));
}

TEST(ComponentTest, UopIdFormat) {
  UopId::Type In;
  In.InstrIndex.Iteration = 1789;
  In.InstrIndex.BBIndex = 42;
  In.UopIndex = 15;
  const std::string Formatted = UopId::Format(In) + "abcd";
  llvm::StringRef Input = Formatted;
  UopId::Type Out;
  ASSERT_FALSE(UopId::Consume(Input, Out));
  ASSERT_EQ(Out.InstrIndex.Iteration, In.InstrIndex.Iteration);
  ASSERT_EQ(Out.InstrIndex.BBIndex, In.InstrIndex.BBIndex);
  ASSERT_EQ(Out.UopIndex, In.UopIndex);
  ASSERT_EQ(Input, "abcd");
}

TEST(ComponentTest, UopIdFormatFail) {
  llvm::StringRef Input;
  UopId::Type Out;

  Input = "abc,10, 12";
  ASSERT_TRUE(UopId::Consume(Input, Out));

  Input = "10,abc,";
  ASSERT_TRUE(UopId::Consume(Input, Out));

  Input = ",45,15";
  ASSERT_TRUE(UopId::Consume(Input, Out));
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
