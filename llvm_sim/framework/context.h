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

// The simulator context provides a way for components to access the LLVM
// context (target-specific info, including instruction descriptions and
// itineraries) and the instructions in the basic block being simulated.

#ifndef EXEGESIS_LLVM_SIM_FRAMEWORK_CONTEXT_H_
#define EXEGESIS_LLVM_SIM_FRAMEWORK_CONTEXT_H_

#include <unordered_map>
#include <vector>

#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/Support/TargetRegistry.h"

namespace exegesis {
namespace simulator {

// Represents the decomposition of an Instruction into Uops.
class InstrUopDecomposition {
 public:
  class Uop {
   public:
    // The ProcRes that this Uop consumes. This can be 0 for uops that don't
    // consume resources, e.g. a register-to-register move on architectures that
    // can rename registers.
    unsigned ProcResIdx = 0;

    // Start/End cycles, relative to the first uop.
    unsigned StartCycle;
    unsigned EndCycle;

    // The execution latency of the Uop.
    unsigned GetLatency() const {
      assert(EndCycle > StartCycle);
      return EndCycle - StartCycle;
    }
  };

  llvm::SmallVector<Uop, 8> Uops;
};

// This is the immutable simulator context which is valid for the lifetime of
// the simulator. It holds information about the LLVM target/subtarget.
// Some functions are virtual to allow easier testing.
class GlobalContext {
 public:
  // Creates the context from the triple and subtarget (`CpuName`) names. This
  // is quite expensive.
  static std::unique_ptr<const GlobalContext> Create(llvm::StringRef TripleName,
                                                     llvm::StringRef CpuName);

  // For tests, use this ctor and fill members manually and/or override methods.
  GlobalContext();

  virtual ~GlobalContext();

  virtual const llvm::MCSchedClassDesc* GetSchedClassForInstruction(
      unsigned Opcode) const;

  // Returns the decomposition of an instruction in uops. The decomposition is
  // computed lazily and cached until the context is destroyed.
  virtual const InstrUopDecomposition& GetInstructionDecomposition(
      unsigned Opcode) const;

  llvm::Triple Triple;
  const llvm::Target* Target = nullptr;

  std::unique_ptr<const llvm::MCInstrInfo> InstrInfo;
  std::unique_ptr<const llvm::MCSubtargetInfo> SubtargetInfo;
  std::unique_ptr<const llvm::MCRegisterInfo> RegisterInfo;
  std::unique_ptr<const llvm::MCAsmInfo> AsmInfo;
  std::unique_ptr<llvm::MCContext> LLVMContext;
  std::unique_ptr<const llvm::MCObjectFileInfo> ObjectFileInfo;
  std::unique_ptr<const llvm::MCCodeEmitter> CodeEmitter;
  const llvm::MCSchedModel* SchedModel = nullptr;

  // For tests.
  void SetInstructionDecomposition(
      unsigned Opcode,
      std::unique_ptr<InstrUopDecomposition> Decomposition) const {
    DecompositionCache_[Opcode] = std::move(Decomposition);
  }

 private:
  class ResourceHierarchy;

  void ComputeInstructionUops(
      const unsigned Opcode,
      llvm::SmallVectorImpl<InstrUopDecomposition::Uop>* Uops) const;
  void ComputeUopLatencies(
      const unsigned Opcode,
      llvm::SmallVectorImpl<InstrUopDecomposition::Uop>* Uops) const;

  // A cache for GetResourceUnitsForUop.
  mutable std::unordered_map<unsigned, std::unique_ptr<InstrUopDecomposition>>
      DecompositionCache_;

  // Mutable because lazily initialized.
  mutable std::unique_ptr<ResourceHierarchy> ResourceHierarchy_;
};

// This is the block context which is valid for a single basic block simulation.
class BlockContext {
 public:
  explicit BlockContext(const std::vector<llvm::MCInst>& Instructions,
                        bool IsLoop);

  // Returns the number of instructions in the basic block.
  size_t GetNumBasicBlockInstructions() const { return Instructions_.size(); }

  // Returns true if this is a perfectly predicted loop body.
  bool IsLoop() const { return IsLoop_; }

  // Return the MCInst for the `BBIndex`-th instruction.
  const llvm::MCInst& GetInstruction(size_t BBIndex) const {
    assert(BBIndex < Instructions_.size());
    return Instructions_[BBIndex];
  }

 private:
  const std::vector<llvm::MCInst>& Instructions_;
  const bool IsLoop_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_FRAMEWORK_CONTEXT_H_
