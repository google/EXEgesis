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

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_BUFFER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_BUFFER_H_

#include <deque>

#include "llvm/Support/Compiler.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"
#include "llvm_sim/framework/log_levels.h"

namespace exegesis {
namespace simulator {

// A buffer of elements of type `InputTag::Type`. The buffer holds elements and
// has a staging area for elements added during the current cycle, which will be
// added during the next Propagate().
// In the following, we denote `[a,b|c,d,e]` the buffer with elements {a, b}
// in the staging area and {c, d, e} in the consumable area. Elements enter on
// the left and exit on the right. [|] is an empty BufferImpl.
template <typename InputTag>
class BufferImpl : public Buffer, public Sink<InputTag> {
 public:
  ~BufferImpl() override {}

  void Init(Logger* Log) override {
    Pending_.clear();
    NumCyclesSinceLastPropagation_ = 0;
  }

  // Adds an input in the buffer in FIFO manner. The input will be made
  // available on the next Propagate() call.
  // [a,b|c,d,e], Elems:[f,g]  ->  [g,f,a,b|c,d,e]
  LLVM_NODISCARD bool PushMany(
      const std::vector<typename InputTag::Type>& Elems) final {
    if (!CanPush(Elems.size(), Pending_.size())) {
      return false;
    }
    for (const auto& Elem : Elems) {
      Pending_.push_front(Elem);
    }
    return true;
  }

  // On propagation, the inputs pushed in the current cycle are made available
  // for consumption if possible.
  // [a,b|c,d,e]  ->  [|a,b,c,d,e]
  void Propagate(Logger* Log) final {
    if (!CanPropagate()) {
      ++NumCyclesSinceLastPropagation_;
      Log->Log("PStall", llvm::Twine(NumCyclesSinceLastPropagation_).str());
      if (NumCyclesSinceLastPropagation_ > 500) {
        std::string Str = "stalled for too long, this is likely a bug.";
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
        llvm::raw_string_ostream OS(Str);
        OS << " Contents: ";
        print(OS);
        OS.flush();
#endif
        Log->Log(LogLevels::kWarning, std::move(Str));
      }
      return;
    }
    NumCyclesSinceLastPropagation_ = 0;
    PrePropagate(Log, Pending_);
    while (!Pending_.empty()) {
      Log->Log(InputTag::kTagName, InputTag::Format(Pending_.back()));
      PropagateImpl(Pending_.back());
      Pending_.pop_back();
    }
  }

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  virtual void print(llvm::raw_ostream& OS) const;
#endif

 protected:
  using QueueT = std::deque<typename InputTag::Type>;

  bool IsStalled() const { return NumCyclesSinceLastPropagation_ > 0; }

 private:
  // Returns true if the buffer will be able to receive at least `N` elements in
  // the next cycle given that there are already `NStaging` in the staging area.
  virtual bool CanPush(size_t N, size_t NStaging) const = 0;

  // Returns true if the buffer is able to accept an element.
  virtual bool CanPropagate() const = 0;

  // Propagates an element. `input` is only valid for the durating of the call.
  // Precondition: CanPropagate() == true.
  virtual void PropagateImpl(const typename InputTag::Type& Input) = 0;

  // Allow subclasses to inspect the elements to be propagated before
  // propagation.
  virtual void PrePropagate(Logger* Log, const QueueT& Pending) {}

  // The inputs pushed during the current cycle.
  QueueT Pending_;
  // The number of cycles since last propagation.
  unsigned NumCyclesSinceLastPropagation_ = 0;
};

// Common functionality for fifo-based buffers.
template <typename ElemTag>
class FifoBufferBase : public BufferImpl<ElemTag>, public Source<ElemTag> {
 public:
  explicit FifoBufferBase(size_t Capacity) : Capacity_(Capacity) {}

  void Init(Logger* Log) override {
    BufferImpl<ElemTag>::Init(Log);
    Fifo_.clear();
  }

  // [a,b|c,d,e]  returns &e
  // [a,b|]       returns nullptr
  const typename ElemTag::Type* Peek() const final {
    return Fifo_.empty() ? nullptr : &Fifo_.back();
  }

  // [a,b|c,d,e] -> [a,b|c,d]
  void Pop() final {
    if (!Fifo_.empty()) {
      Fifo_.pop_back();
    }
  }

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  void print(llvm::raw_ostream& OS) const override;
#endif

 protected:
  size_t Capacity() const { return Capacity_; }
  // [a,b|c,d,e] returns 3
  size_t Size() const { return Fifo_.size(); }

 private:
  void PropagateImpl(const typename ElemTag::Type& Input) override {
    Fifo_.push_front(Input);
    assert(Fifo_.size() <= Capacity_);
  }

  const size_t Capacity_;
  std::deque<typename ElemTag::Type> Fifo_;
};

// A Simple FIFO buffer. Elements are made available in the order in which they
// were added.
template <typename ElemTag>
class FifoBuffer : public FifoBufferBase<ElemTag> {
 public:
  explicit FifoBuffer(size_t Capacity) : FifoBufferBase<ElemTag>(Capacity) {}

 private:
  bool CanPush(size_t N, size_t NStaging) const override {
    return N + NStaging + this->Size() <= this->Capacity();
  }

  bool CanPropagate() const final {
    assert(this->Size() <= this->Capacity());
    return true;
  }
};

// A LinkBuffer is simply a buffer connecting two components where the source
// component will stall if the second has not consumed all the elements that the
// former pushed in the previous cycle.
template <typename ElemTag>
class LinkBuffer : public FifoBufferBase<ElemTag> {
 public:
  explicit LinkBuffer(size_t Capacity) : FifoBufferBase<ElemTag>(Capacity) {}

  ~LinkBuffer() override {}

 private:
  bool CanPropagate() const final {
    // Can only propagate when the consumer component has consumed all elements.
    return this->Peek() == nullptr;
  }

  bool CanPush(size_t N, size_t NStaging) const final {
    return
        // We can only push new elements if we're not currently stalled.
        !this->IsStalled() &&
        // We can only push `Capacity` elements in the staging area.
        N + NStaging <= this->Capacity();
  }
};

// A DevNullBuffer is a buffer that discards all the input. It's mostly useful
// for logging.
template <typename InputTag>
class DevNullBuffer : public BufferImpl<InputTag> {
 public:
  ~DevNullBuffer() override {}

 private:
  bool CanPush(size_t N, size_t NStaging) const final { return true; }

  bool CanPropagate() const final { return true; }

  void PropagateImpl(const typename InputTag::Type& Input) final {}
};

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
template <typename InputTag>
void BufferImpl<InputTag>::print(llvm::raw_ostream& OS) const {
  OS << "[";
  bool First = true;
  for (const auto& Elem : Pending_) {
    if (First) {
      First = false;
    } else {
      OS << ",";
    }
    OS << "(" << InputTag::Format(Elem) << ")";
  }
  OS << "|";
}
#endif

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
template <typename InputTag>
void FifoBufferBase<InputTag>::print(llvm::raw_ostream& OS) const {
  BufferImpl<InputTag>::print(OS);
  bool First = true;
  for (const auto& Elem : Fifo_) {
    if (First) {
      First = false;
    } else {
      OS << ",";
    }
    OS << "(" << InputTag::Format(Elem) << ")";
  }
  OS << "]";
}
#endif

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_BUFFER_H_
