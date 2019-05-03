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

#ifndef EXEGESIS_LLVM_SIM_FRAMEWORK_COMPONENT_H_
#define EXEGESIS_LLVM_SIM_FRAMEWORK_COMPONENT_H_

#include "llvm_sim/framework/context.h"

namespace exegesis {
namespace simulator {

// Simulator components implement this interface.
class Component {
 public:
  explicit Component(const GlobalContext* const Context);

  virtual ~Component();

  // The following functions are only called by the framework or in tests.
  // The framework ensures that for each cycle, Tick() is first called on all
  // components, then Propagate() is called on all buffers.

  // Called by the framework before starting the simulation of a basic block.
  virtual void Init() {}
  // Called for each clock cycle. This is typically where components read and
  // write from inputs/outputs.
  virtual void Tick(const BlockContext* BlockContext) = 0;

 protected:
  const GlobalContext& Context;
};

class Logger;

// Components output elements (e.g. instructions, uops, ...) to `Buffers`.
// Buffers have a staging area for the current cycle, so that the contents
// Push()ed by a component during the current cycle are made available
// downstream only during the next cycle. This happens when Propagate() is
// called.
class Buffer {
 public:
  virtual ~Buffer();

  // Called by the framework before starting the simulation of a basic block.
  // `Log` is valid only during the duration of the call.
  virtual void Init(Logger* Log) = 0;

  // Called by the framework after each complete clock cycle.
  // On propagation, the inputs pushed in the current cycle are made available
  // for consumption.
  // `Log` is valid only during the duration of the call.
  virtual void Propagate(Logger* Log) = 0;
};

// The interface used by buffers to report state changes.
class Logger {
 public:
  virtual ~Logger();
  virtual void Log(std::string MsgTag, std::string Msg) = 0;
};

// The interfaces for pushing elements to a component or buffer. `Tag` is the
// element tag, used to statically check that component/buffers are correctly
// plugged. Components are free to define their own tags.
//
// Tags should define:
//   - a type `Type`
//   - a name for `kTagName`.
//   - `static std:string Format(const Type&)` that formats a type for logging.

template <typename Tag>
class Sink {
 public:
  virtual ~Sink() {}

  // Returns true if the element was pushed.
  LLVM_NODISCARD bool Push(const typename Tag::Type& Elem) {
    return PushMany({Elem});
  }

  // Atomically push a bunch of elements (either all or no elements are pushed).
  // Returns true if all the elements were pushed.
  LLVM_NODISCARD virtual bool PushMany(
      const std::vector<typename Tag::Type>& Elems) = 0;
};

template <typename Tag>
class Source {
 public:
  virtual ~Source() {}

  // Returns the first available element or nullptr if empty.
  virtual const typename Tag::Type* Peek() const = 0;

  // Pops the first element.
  virtual void Pop() = 0;
};

// `InstructionIndex` is a basic tag that is used by the simulator and a most of
// the components.
struct InstructionIndex {
  struct Type {
    size_t BBIndex;    // Instruction indices in the basic block.
    size_t Iteration;  // Loop iteration.
  };

  static const char kTagName[];
  static std::string Format(const Type& Elem);
  static const InstructionIndex::Type& GetInstructionIndex(const Type& Elem) {
    return Elem;
  }
  // Consumes an element from `Input`. Returns false on success.
  static bool Consume(llvm::StringRef& Input, Type& Elem);
};

// `UopId` represents a uop inside an instruction.
struct UopId {
  struct Type {
    InstructionIndex::Type InstrIndex;
    size_t UopIndex;  // The index of the Uop within this instruction's Uops.
  };

  static const char kTagName[];
  static std::string Format(const Type& Elem);
  static const InstructionIndex::Type& GetInstructionIndex(const Type& Elem) {
    return Elem.InstrIndex;
  }
  // Consumes an element from `Input`. Returns false on success.
  static bool Consume(llvm::StringRef& Input, Type& Elem);
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_FRAMEWORK_COMPONENT_H_
