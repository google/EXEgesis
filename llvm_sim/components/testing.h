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

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_TESTING_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_TESTING_H_

#include <deque>
#include <limits>
#include <vector>

#include "gmock/gmock.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/reorder_buffer.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// A test sink that allows direct access to the underlying buffer.
template <typename Tag>
class TestSink : public Sink<Tag> {
 public:
  TestSink() { SetInfiniteCapacity(); }

  bool PushMany(const std::vector<typename Tag::Type>& Elems) override {
    if (Buffer_.size() + Elems.size() > Capacity_) {
      return false;
    }
    for (const auto& Elem : Elems) {
      Buffer_.push_back(Elem);
    }
    return true;
  }

  void SetCapacity(size_t Capacity) { Capacity_ = Capacity; }
  void SetInfiniteCapacity() { SetCapacity(std::numeric_limits<int>::max()); }

  std::vector<typename Tag::Type> Buffer_;

 private:
  size_t Capacity_;
};

// A test source that allows direct access to the underlying buffer.
template <typename Tag>
class TestSource : public Source<Tag> {
 public:
  virtual const typename Tag::Type* Peek() const {
    if (Buffer_.empty()) {
      return nullptr;
    }
    return &Buffer_.front();
  }

  virtual void Pop() { Buffer_.pop_front(); }

  std::deque<typename Tag::Type> Buffer_;
};

class MockLogger : public Logger {
 public:
  MOCK_METHOD2(Log, void(std::string MsgTag, std::string Msg));
};

MATCHER_P2(EqInstrIndex, Iteration, BBIndex, "") {
  if (arg.Iteration == Iteration && arg.BBIndex == BBIndex) {
    return true;
  }
  *result_listener << "instr id(iter=" << arg.Iteration
                   << ", bbindex=" << arg.BBIndex << ")";
  return false;
}

MATCHER_P3(EqUopId, Iteration, BBIndex, UopIndex, "") {
  if (arg.InstrIndex.Iteration == Iteration &&
      arg.InstrIndex.BBIndex == BBIndex && arg.UopIndex == UopIndex) {
    return true;
  }
  *result_listener << "uop id (iter=" << arg.InstrIndex.Iteration
                   << ", bbindex=" << arg.InstrIndex.BBIndex
                   << ", uopid=" << arg.UopIndex << ")";
  return false;
}

struct TestInputTag {
  using Type = int;
  static std::string Format(const Type& Elem) { return std::to_string(Elem); }
  static const char kTagName[];
  static InstructionIndex::Type GetInstructionIndex(const Type& Elem) {
    InstructionIndex::Type InstrIndex;
    InstrIndex.BBIndex = static_cast<size_t>(2 * Elem);
    InstrIndex.Iteration = 42;
    return InstrIndex;
  }
};

// Returns an InstructionIndex at iteration 0 with the given BBIndex.
InstructionIndex::Type TestInstrIndex(size_t BBIndex);

struct TestExecutionUnitInputTag {
  struct Type {
    int Id;
    unsigned Latency;
  };
  static const char kTagName[];
  static std::string Format(const Type& Elem) {
    return std::to_string(Elem.Id);
  }
};

MATCHER_P(HasId, Id, "") {
  if (arg.Id == Id) {
    return true;
  }
  *result_listener << "id=" << arg.Id;
  return false;
}

// Builders for common types:

template <typename T>
class BuilderBase {
 public:
  typename T::Type Build() const { return Value_; }

 protected:
  typename T::Type Value_;
};

class RenamedUopIdBuilder : public BuilderBase<RenamedUopId> {
 public:
  RenamedUopIdBuilder() {
    // Default to iteration 0.
    Value_.Uop.InstrIndex.Iteration = 0;
  }

  RenamedUopIdBuilder& WithIter(size_t Iter) {
    Value_.Uop.InstrIndex.Iteration = Iter;
    return *this;
  }

  RenamedUopIdBuilder& WithUop(const UopId::Type& Uop) {
    Value_.Uop = Uop;
    return *this;
  }

  RenamedUopIdBuilder& WithUop(size_t BBIndex, size_t UopIndex) {
    Value_.Uop.InstrIndex.BBIndex = BBIndex;
    Value_.Uop.UopIndex = UopIndex;
    return *this;
  }

  RenamedUopIdBuilder& AddUse(size_t Use) {
    Value_.Uses.push_back(Use);
    return *this;
  }

  RenamedUopIdBuilder& AddDef(size_t Def) {
    Value_.Defs.push_back(Def);
    return *this;
  }
};

class ROBUopIdBuilder : public BuilderBase<ROBUopId> {
 public:
  ROBUopIdBuilder() {
    // Default to iteration 0.
    Value_.Uop.InstrIndex.Iteration = 0;
  }

  ROBUopIdBuilder& WithIter(size_t Iter) {
    Value_.Uop.InstrIndex.Iteration = Iter;
    return *this;
  }

  ROBUopIdBuilder& WithUop(const UopId::Type& Uop) {
    Value_.Uop = Uop;
    return *this;
  }

  ROBUopIdBuilder& WithUop(size_t BBIndex, size_t UopIndex) {
    Value_.Uop.InstrIndex.BBIndex = BBIndex;
    Value_.Uop.UopIndex = UopIndex;
    return *this;
  }

  ROBUopIdBuilder& WithEntryIndex(size_t ROBEntryIndex) {
    Value_.ROBEntryIndex = ROBEntryIndex;
    return *this;
  }
};

// Checks that the buffer contents in debug mode.
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
#define CHECK_BUFFER_CONTENTS(buffer, expected) \
  do {                                          \
    std::string Actual;                         \
    llvm::raw_string_ostream OS(Actual);        \
    (buffer).print(OS);                         \
    OS.flush();                                 \
    ASSERT_EQ(Actual, (expected));              \
  } while (0)
#else
#define CHECK_BUFFER_CONTENTS(buffer, expected)
#endif

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_TESTING_H_
