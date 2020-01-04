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

#include "llvm_sim/components/register_renamer.h"

#include "llvm/MC/MCInstrItineraries.h"

namespace exegesis {
namespace simulator {

namespace {

class RegisterNameTrackerImpl : public RegisterNameTracker {
 public:
  explicit RegisterNameTrackerImpl(const llvm::MCRegisterInfo& RegisterInfo)
      : RegisterInfo_(RegisterInfo), Names_(RegisterInfo.getNumRegUnits()) {}

  void SetName(const unsigned Reg, const size_t Name) override {
    assert(Name > 0);
    for (llvm::MCRegUnitIterator Unit(Reg, &RegisterInfo_); Unit.isValid();
         ++Unit) {
      Names_[*Unit] = Name;
    }
  }

  llvm::SmallVector<size_t, 4> GetNameDeps(const unsigned Reg) const override {
    llvm::SmallVector<size_t, 4> Result;
    for (llvm::MCRegUnitIterator Unit(Reg, &RegisterInfo_); Unit.isValid();
         ++Unit) {
      if (const size_t Name = Names_[*Unit]) {
        if (std::find(Result.begin(), Result.end(), Name) == Result.end()) {
          Result.push_back(Name);
        }
      }
    }
    return Result;
  }

  void Reset() override { std::fill(Names_.begin(), Names_.end(), 0); }

 private:
  const llvm::MCRegisterInfo& RegisterInfo_;
  // Names, indexed by register, or 0 if there is no active name for this
  // register (there might be active names for subregisters).
  std::vector<size_t> Names_;
};
}  // namespace

std::unique_ptr<RegisterNameTracker> RegisterNameTracker::Create(
    const llvm::MCRegisterInfo& RegisterInfo) {
  return absl::make_unique<RegisterNameTrackerImpl>(RegisterInfo);
}

RegisterNameTracker::~RegisterNameTracker() {}

RegisterRenamer::RegisterRenamer(const GlobalContext* Context,
                                 const Config& Config, Source<UopId>* Source,
                                 Sink<RenamedUopId>* Sink)
    : RegisterRenamer(Context, Config, Source, Sink,
                      RegisterNameTracker::Create(*Context->RegisterInfo)) {}

RegisterRenamer::RegisterRenamer(const GlobalContext* Context,
                                 const Config& Config, Source<UopId>* Source,
                                 Sink<RenamedUopId>* Sink,
                                 std::unique_ptr<RegisterNameTracker> Tracker)
    : Component(Context),
      Config_(Config),
      Source_(Source),
      Sink_(Sink),
      FirstPhysicalRegisterId_(Context->RegisterInfo->getNumRegs() + 1),
      Tracker_(std::move(Tracker)) {
  assert(Config_.UopsPerCycle > 0);
}

RegisterRenamer::~RegisterRenamer() {}

void RegisterRenamer::Init() {
  PhysicalRegistersFreelist_.clear();
  NumAllocatedPhysicalRegisters_ = 0;
  HasPendingUop_ = false;
  Tracker_->Reset();
}

void RegisterRenamer::Tick(const BlockContext* BlockContext) {
  // TODO(courbet): This is where `XOR EAX, EAX` detection is implemented.
  // Also: XOR, SUB, PXOR, XORPS, XORPD, VXORPS, VXORPD and all variants
  // of PSUBxxx and PCMPGTxx.
  unsigned RemainingUops = Config_.UopsPerCycle;

  // We might have a pending renamed uop, flush it.
  if (HasPendingUop_) {
    if (!Sink_->Push(RenamedUop_)) {
      return;
    }
    HasPendingUop_ = false;
  }

  while (const UopId::Type* Uop = Source_->Peek()) {
    if (!PopulateUop(BlockContext, *Uop)) {
      // Could not rename one or more registers, retry the uop next time.
      return;
    }
    Source_->Pop();
    if (!Sink_->Push(RenamedUop_)) {
      // Mark the uop as pending an try it on the next tick.
      HasPendingUop_ = true;
      return;
    }
    if (--RemainingUops == 0) {
      return;
    }
  }
}

bool RegisterRenamer::PopulateUop(const BlockContext* BlockContext,
                                  const UopId::Type& Uop) {
  // Reset the Uop.
  RenamedUop_ = RenamedUopId::Type{Uop};

  // TODO(courbet): Right now for lack of better information we assume that the
  // first uop of an instruction reads all the `uses` and the last uop writes
  // all the `defs`. Use the information about read/write latencies to assign
  // use/defs to uops.
  if (Uop.UopIndex == 0) {
    HandleFirstUop(BlockContext);
  }

  const auto& Decomposition = Context.GetInstructionDecomposition(
      BlockContext->GetInstruction(Uop.InstrIndex.BBIndex));
  assert(!Decomposition.Uops.empty());
  if (Uop.UopIndex == Decomposition.Uops.size() - 1) {
    return HandleLastUop(BlockContext);
  }
  return true;
}

void RegisterRenamer::HandleFirstUop(const BlockContext* BlockContext) {
  const llvm::MCInst& Inst =
      BlockContext->GetInstruction(RenamedUop_.Uop.InstrIndex.BBIndex);
  const auto& InstrDesc = Context.InstrInfo->get(Inst.getOpcode());

  const auto AddUse = [this](const unsigned Reg) {
    for (const size_t Name : Tracker_->GetNameDeps(Reg)) {
      if (std::find(RenamedUop_.Uses.begin(), RenamedUop_.Uses.end(), Name) ==
          RenamedUop_.Uses.end()) {
        RenamedUop_.Uses.push_back(Name);
      }
    }
  };

  // Explicit uses. LLVM store explicit defs, then explicit uses. We just skip
  // over defs.
  for (unsigned I = InstrDesc.getNumDefs(); I < Inst.getNumOperands(); ++I) {
    const llvm::MCOperand& Op = Inst.getOperand(I);
    assert(Op.isValid());
    if (Op.isReg() && Op.getReg() != 0) {
      AddUse(Op.getReg());
    }
  }
  // Implicit uses:
  for (const llvm::MCPhysReg* Reg = InstrDesc.getImplicitUses();
       Reg && *Reg != 0; ++Reg) {
    AddUse(*Reg);
  }
}

bool RegisterRenamer::HandleLastUop(const BlockContext* BlockContext) {
  const llvm::MCInst& Inst =
      BlockContext->GetInstruction(RenamedUop_.Uop.InstrIndex.BBIndex);
  const auto& InstrDesc = Context.InstrInfo->get(Inst.getOpcode());

  const auto ForEachRegisterDef =
      [&Inst, &InstrDesc](const std::function<void(unsigned Reg)>& Callback) {
        // Explicit defs.
        for (unsigned I = 0; I < InstrDesc.getNumDefs(); ++I) {
          const llvm::MCOperand& Op = Inst.getOperand(I);
          assert(Op.isValid());
          if (Op.isReg()) {
            Callback(Op.getReg());
          }
        }
        // Implicit defs:
        for (const llvm::MCPhysReg* Reg = InstrDesc.getImplicitDefs();
             Reg && *Reg != 0; ++Reg) {
          Callback(*Reg);
        }
      };

  // First pass to gather how many registers we need to handle the uops. We only
  // want to start modifying the state of this class when we can finish the
  // uop.
  unsigned NumRenames = 0;
  ForEachRegisterDef([this, &NumRenames](unsigned Reg) {
    if (CanBeRenamed(Reg)) {
      ++NumRenames;
    }
  });
  if (!HasAtLeastFreePhysicalRegisterIds(NumRenames)) {
    return false;
  }
  // Second pass to actually do the renames.
  ForEachRegisterDef([this](unsigned Reg) {
    const size_t PhysReg =
        CanBeRenamed(Reg) ? GetFreePhysicalRegisterId() : Reg;
    assert(PhysReg > 0);
    Tracker_->SetName(Reg, PhysReg);
    RenamedUop_.Defs.push_back(PhysReg);
  });
  return true;
}

size_t RegisterRenamer::GetFreePhysicalRegisterId() {
  // Use registers from the freelist in priority.
  if (!PhysicalRegistersFreelist_.empty()) {
    const auto Result = PhysicalRegistersFreelist_.back();
    PhysicalRegistersFreelist_.pop_back();
    return Result;
  }
  // Else allocate a new register until the max number of registers is hit.
  if (NumAllocatedPhysicalRegisters_ == Config_.NumPhysicalRegisters) {
    return 0;
  }
  ++NumAllocatedPhysicalRegisters_;
  return FirstPhysicalRegisterId_ + NumAllocatedPhysicalRegisters_;
}

bool RegisterRenamer::HasAtLeastFreePhysicalRegisterIds(const size_t N) {
  return NumAllocatedPhysicalRegisters_ + N <
         Config_.NumPhysicalRegisters + PhysicalRegistersFreelist_.size();
}

void RegisterRenamer::ReleasePhysicalRegisterId(size_t Id) {
  PhysicalRegistersFreelist_.push_back(Id);
}

bool RegisterRenamer::CanBeRenamed(unsigned Reg) const {
  // TODO(courbet): Add this information to the LLVM schema and improve.
  return true;
}

}  // namespace simulator
}  // namespace exegesis
