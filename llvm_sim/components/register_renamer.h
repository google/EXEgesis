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

// A register renamer.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_REGISTER_RENAMER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_REGISTER_RENAMER_H_

#include <unordered_map>

#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// This class allows tracking register names.
// Registers can alias. For example, we need to handle cases like:
//   mov AX  <- 42   # (1)
//   mov AH  <- 43   # (2)
//   mov AL  <- 44   # (3)
//   mov EBX <- EAX  # (4)
// (4) depends on the values in AX from (3) and (2).
// For each register, tracks the set of `names` it depends on.
// This models the fact that names are backed up by hardware registers that
// share memory regions.
// A read of the register depends on all its previously defined names.
// When writing to a register through a physical register, we always set the
// value of all bits in the register. For example, renaming EAX to $N means that
// reads to EAX, AX, AL and AH now depend on $N. Subsequently renaming AL to $M
// means that reads of EAX now depend on $N and $M.
// In the following diagrams, we write in parenthesis the last renaming of a
// register. The list of name dependencies of a register is the union of its
// name as well as the names of all its subregisters.
//
// When a register gets renamed, we update its names as well as that of its
// subregisters.
// For example:
//   1 - When renaming to AH to $5, we update AH. AH has no subregisters, so
//       nothing needs to be updated.
//                              /-->AL()
//        RAX()--->EAX()--->AX()
//                              \-->AH($5)
//   2 - When subsequently renaming AX to $6, we update AX, but also clear all
//       its subregisters (AL, AH) names.
//                               /-->AL()
//        RAX()--->EAX()--->AX($6)
//                               \-->AH()
//   3 - Let's rename AH again to $7, the tree is updated to look like:
//                               /-->AL()
//        RAX()--->EAX()--->AX($6)
//                               \-->AH($7)
//       Note that AX is not cleared, because users of AX or AL need the value
//       of AL which is set by $6.
//
//   4 - If subsequently renaming AL to $8, we update AL, but also clear all
//       superregisters that are fully covered by subregisters (AX in this
//       case).
//                              /-->AL($8)
//        RAX()--->EAX()--->AX()
//                              \-->AH($7)
class RegisterNameTracker {
 public:
  static std::unique_ptr<RegisterNameTracker> Create(
      const llvm::MCRegisterInfo& RegisterInfo);

  virtual ~RegisterNameTracker();

  // Sets the name of `Reg` to Name (declares that writes to Reg will be done
  // through Name).
  virtual void SetName(unsigned Reg, size_t Name) = 0;

  // Returns all the names that `Reg` depends upon. The list of name
  // dependencies for a register is the union of the names for all its register
  // units.
  // In example (1): GetNameDeps(RAX, EAX, AX, AH) = {5}; GetNameDeps(AL) = {}.
  // In example (2): GetNameDeps(RAX, EAX, AX, AH, AL) = {6}.
  // In example (3): GetNameDeps(RAX, EAX, AX, AH) = {6,7}; GetNameDeps(AL) =
  //                 {}.
  // In example (4): GetNameDeps(RAX, EAX, AX) = {7,8}; GetNameDeps(AL) = {7},
  //                 GetNameDeps(AX) = {8}.
  virtual llvm::SmallVector<size_t, 4> GetNameDeps(unsigned Reg) const = 0;

  virtual void Reset() {}
};

class RegisterRenamer : public Component {
 public:
  struct Config {
    // The number of uops whose registers can be renamed per cycle.
    size_t UopsPerCycle;
    // The number of available physical (a.k.a microarchitectural) registers.
    size_t NumPhysicalRegisters;
  };

  RegisterRenamer(const GlobalContext* Context, const Config& Config,
                  Source<UopId>* Source, Sink<RenamedUopId>* Sink);

  // For tests.
  RegisterRenamer(const GlobalContext* Context, const Config& Config,
                  Source<UopId>* Source, Sink<RenamedUopId>* Sink,
                  std::unique_ptr<RegisterNameTracker> Tracker);

  ~RegisterRenamer() override;

  void Init() override;
  void Tick(const BlockContext* BlockContext) override;

 private:
  // Returns a free physical register id, or 0 if none are available.
  size_t GetFreePhysicalRegisterId();
  // Returns true if there are at lest that many available register ids.
  bool HasAtLeastFreePhysicalRegisterIds(size_t N);
  // Releases a given register id.
  void ReleasePhysicalRegisterId(size_t Id);
  // Returns true if this register can be renamed.
  bool CanBeRenamed(unsigned Reg) const;

  // Helpers to handle a single Uop.
  bool PopulateUop(const BlockContext* BlockContext, const UopId::Type& Uop);
  void HandleFirstUop(const BlockContext* BlockContext);
  bool HandleLastUop(const BlockContext* BlockContext);

  const Config Config_;
  Source<UopId>* const Source_;
  Sink<RenamedUopId>* const Sink_;
  // The first physical register id.
  const size_t FirstPhysicalRegisterId_;

  std::unique_ptr<RegisterNameTracker> Tracker_;
  // The list of available physical register ids.
  std::vector<size_t> PhysicalRegistersFreelist_;
  // The number of physical registers ever allocated. All physical registers ids
  // below this are either in the freelist or in use.
  size_t NumAllocatedPhysicalRegisters_;

  // The current renamed Uop.
  RenamedUopId::Type RenamedUop_;
  // Is there a pending uop waiting to be flushed ?
  bool HasPendingUop_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_REGISTER_RENAMER_H_
