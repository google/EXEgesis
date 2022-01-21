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

#include "llvm_sim/framework/context.h"

#include "llvm/ADT/BitVector.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCObjectFileInfo.h"

namespace exegesis {
namespace simulator {

// A class to keep track of which resources are fully contained in other
// resources. This is to "undo" the denormalization that happens when TableGen
// backends generate MCWriteProcResEntry's.
// TODO(courbet): This data should be accessible in LLVM data structures
// directly. Ideally we would expose super resources and denormalize on the
// fly using an iterator. This will make generated tables smaller but might
// hurt performance a little bit due to the extra abstraction.
class GlobalContext::ResourceHierarchy {
 public:
  explicit ResourceHierarchy(const llvm::MCSchedModel& SchedModel);

  // Returns the resources in which resource `ProcResourceIdx` is fully
  // contained.
  const std::vector<unsigned>& GetSuperResources(
      unsigned ProcResourceIdx) const {
    return SuperResources_[ProcResourceIdx];
  }

 private:
  std::vector<std::vector<unsigned>> SuperResources_;
};

GlobalContext::ResourceHierarchy::ResourceHierarchy(
    const llvm::MCSchedModel& SchedModel) {
  const auto NumProcResources = SchedModel.getNumProcResourceKinds();
  // BaseMasks[I] has bit J set iff proc resource I has ProcResource J as base
  // resource. For example, if P0, P5 and P05 have ids 1, 3, and 7,
  //    BaseMasks[1] == 1 << 1
  //    BaseMasks[3] == 1 << 3
  //    BaseMasks[7] == (1 << 1) | (1 << 3)
  std::vector<llvm::BitVector> BaseMasks(NumProcResources,
                                         llvm::BitVector(NumProcResources));
  for (size_t I = 1; I < NumProcResources; ++I) {
    const llvm::MCProcResourceDesc* const ProcResDesc =
        SchedModel.getProcResource(I);
    if (ProcResDesc->SubUnitsIdxBegin == nullptr) {
      // This is a ProcResUnit.
      BaseMasks[I].set(I);
    } else {
      // This is a ProcResGroup.
      for (const auto* SubResIdx = ProcResDesc->SubUnitsIdxBegin;
           SubResIdx != ProcResDesc->SubUnitsIdxBegin + ProcResDesc->NumUnits;
           ++SubResIdx) {
        BaseMasks[I].set(*SubResIdx);
      }
    }
  }
  // This is N^2, but N is small.
  // TODO(courbet): Make it N if this is an issue.
  SuperResources_.resize(NumProcResources);
  for (size_t I = 1; I < NumProcResources; ++I) {
    for (size_t J = 1; J < NumProcResources; ++J) {
      if (I == J) continue;
      auto Intersection = BaseMasks[I];
      Intersection &= BaseMasks[J];
      if (Intersection == BaseMasks[I]) {
        // I is fully contained in J.
        SuperResources_[I].push_back(J);
      }
    }
  }
}

std::unique_ptr<const GlobalContext> GlobalContext::Create(
    llvm::StringRef TripleName, llvm::StringRef CpuName) {
  return CreateMutable(TripleName, CpuName);
}

std::unique_ptr<GlobalContext> GlobalContext::CreateMutable(
    llvm::StringRef TripleName, llvm::StringRef CpuName) {
  std::string ErrorMsg;
  const auto* const Target =
      llvm::TargetRegistry::lookupTarget(std::string(TripleName), ErrorMsg);
  if (!Target) {
    llvm::errs() << "cannot create target for triple '" << TripleName
                 << "': " << ErrorMsg << "\n";
    return {};
  }

  auto Context = absl::make_unique<GlobalContext>();
  Context->Target = Target;
  Context->Triple = llvm::Triple(TripleName);
  Context->InstrInfo.reset(Target->createMCInstrInfo());
  Context->SubtargetInfo.reset(
      Target->createMCSubtargetInfo(TripleName, CpuName, ""));
  Context->RegisterInfo.reset(Target->createMCRegInfo(TripleName));
  Context->AsmInfo.reset(Target->createMCAsmInfo(
      *Context->RegisterInfo, Context->Triple.str(), llvm::MCTargetOptions()));
  Context->LLVMContext = absl::make_unique<llvm::MCContext>(
      Context->Triple, Context->AsmInfo.get(), Context->RegisterInfo.get(),
      Context->SubtargetInfo.get());
  Context->ObjectFileInfo.reset(
      Target->createMCObjectFileInfo(*Context->LLVMContext, /*PIC=*/false));
  Context->LLVMContext->setObjectFileInfo(Context->ObjectFileInfo.get());
  Context->CodeEmitter.reset(Target->createMCCodeEmitter(
      *Context->InstrInfo, *Context->RegisterInfo, *Context->LLVMContext));
  Context->SchedModel = &Context->SubtargetInfo->getSchedModel();

  return Context;
}

GlobalContext::GlobalContext() {}

GlobalContext::~GlobalContext() {}

const llvm::MCSchedClassDesc* GlobalContext::GetSchedClassForInstruction(
    const llvm::MCInst& Inst) const {
  assert(InstrInfo && "InstrInfo is required");
  assert(SchedModel && "SchedModel is required");
  // Get the definition's scheduling class descriptor from this machine model.
  assert(Inst.getOpcode() != 0 && "invalid opcode");
  unsigned SchedClassId = InstrInfo->get(Inst.getOpcode()).getSchedClass();
  const llvm::MCSchedClassDesc* SCDesc =
      SchedModel->getSchedClassDesc(SchedClassId);
  assert(SCDesc && "missing sched class");
  while (SCDesc->isVariant()) {
    SchedClassId = SubtargetInfo->resolveVariantSchedClass(
        SchedClassId, &Inst, InstrInfo.get(), SchedModel->getProcessorID());
    SCDesc = SchedModel->getSchedClassDesc(SchedClassId);
    assert(SCDesc && "missing sched class");
  }
  assert(SCDesc->isValid() && "invalid sched class");
  return SCDesc;
}

void GlobalContext::ComputeInstructionUops(
    const llvm::MCInst& Inst,
    llvm::SmallVectorImpl<InstrUopDecomposition::Uop>* Uops) const {
  // TODO(courbet): Handle scheduling models that have itineraries instead of
  // sched classes.
  const auto* const SCDesc = GetSchedClassForInstruction(Inst);

  // The scheduling model for LLVM is such that each instruction has a certain
  // number of uops which consume resources which are described by
  // WriteProcRes entries. Each entry describe how many cycles are spent on
  // a specific ProcRes kind.
  // For example, an instruction might have 3 uOps, one dispatching on P0
  // (ProcResIdx=1) and two on P06 (ProcResIdx = 7).
  // Note that LLVM additionally denormalizes resource consumption to include
  // usage of super resources by subresources. So in practice if there exists
  // a P016 (ProcResIdx=10), then the cycles consumed by P0 are also consumed
  // by P06 and P016, and the resources consumed by P06 are also consumed by
  // P016. In the figure below, parenthesized cycles denote implied usage of
  // superresources by subresources:
  //            P0      P06      P016
  //     uOp1    1      (1)       (1)
  //     uOp2            1        (1)
  //     uOp3            1        (1)
  //     =============================
  //             1       3         3
  // Eventually we end up with three entries for the WriteProcRes of the
  // instruction:
  //    {ProcResIdx=1,  Cycles=1}  // P0
  //    {ProcResIdx=7,  Cycles=3}  // P06
  //    {ProcResIdx=10, Cycles=3}  // P016
  //
  // TODO(courbet): At the time of writing, there a a few issues with
  // scheduling info:
  //   1 - Ordering of resource usage is arbitrary in the TD file, and the
  //       information is not propagated.
  //   2 - Some instructions are missing ResourceCycles entries in the TD
  //       file, and the emitter assumes 1 cycle on every resource (which
  //       breaks the invariant sum(ResourceCycles) == NumMicroOps).

  // Populate the number of cycles per resource.
  llvm::SmallVector<unsigned, 64> ProcResIdxToCycles;
  ProcResIdxToCycles.resize(SchedModel->getNumProcResourceKinds(), 0);
  for (const auto* WPR = SubtargetInfo->getWriteProcResBegin(SCDesc);
       WPR != SubtargetInfo->getWriteProcResEnd(SCDesc); ++WPR) {
    assert(ProcResIdxToCycles[WPR->ProcResourceIdx] == 0 &&
           "WriteProcRes reference the same ProcResIdx multiple times");
    ProcResIdxToCycles[WPR->ProcResourceIdx] = WPR->Cycles;
  }

  // Consume resources one by one, and undenormalize (undo denormalization of
  // resources to super resources). This assumes that WriteProcRes are sorted
  // in Topological order, which is guaranteed by the TableGen backend.
  for (int ProcResIdx = 1; ProcResIdx < SchedModel->getNumProcResourceKinds();
       ++ProcResIdx) {
    const unsigned NumCycles = ProcResIdxToCycles[ProcResIdx];
    if (NumCycles == 0) {
      continue;
    }
    // Un-denormalize resource usage to avoid double-counting.
    for (const unsigned SuperProcResIdx :
         ResourceHierarchy_->GetSuperResources(ProcResIdx)) {
      assert(ProcResIdxToCycles[SuperProcResIdx] >= NumCycles);
      ProcResIdxToCycles[SuperProcResIdx] -= NumCycles;
    }
    // Emit `NumCycles` Uops that consume this resource during one cycle.
    for (unsigned I = 0; I < NumCycles; ++I) {
      InstrUopDecomposition::Uop Uop;
      Uop.ProcResIdx = ProcResIdx;
      Uops->emplace_back(Uop);
    }
    ProcResIdxToCycles[ProcResIdx] = 0;
  }

  // Some instructions emit uops that do NOT consume a resource but are still
  // processed by pipeline stages. These include, for example, register-to-
  // register moves on post-haswell Intel chips. The move will happen when
  // renaming (no execution unit is required), but a uop will still be emitted
  // so that retirement can happen (see e.g.
  // https://patents.google.com/patent/WO2013101323A1/en).
  if (Uops->size() != SCDesc->NumMicroOps) {
    // TODO(courbet): Right now one-uop instructions are the only ones we know
    // of (register-to-register moves). We still warn about these cases
    // because we want to check them.
#if !defined(NDEBUG)
    // These are TD mistakes that should be fixed in LLVM.
    llvm::errs() << InstrInfo->getName(Inst.getOpcode())
                 << ": inconsistent sum(ResourceCycles) (" << Uops->size()
                 << ") vs NumMicroOps (" << SCDesc->NumMicroOps << ")\n";
#endif
    if (Uops->empty() && SCDesc->NumMicroOps == 1) {
#if !defined(NDEBUG)
      llvm::errs() << InstrInfo->getName(Inst.getOpcode())
                   << ": assuming resourceless uop\n";
#endif
      // Add a resourceless uop.
      InstrUopDecomposition::Uop Uop;
      Uop.ProcResIdx = 0;
      Uops->push_back(Uop);
    }
  }
}

void GlobalContext::ComputeUopLatencies(
    const llvm::MCInst& Inst,
    llvm::SmallVectorImpl<InstrUopDecomposition::Uop>* Uops) const {
  const auto* const SCDesc = GetSchedClassForInstruction(Inst);

  // We assume that the latency of the instruction is the maximum time it takes
  // for all its defs to be written.
  int MaxDefLatency = 1;
  for (int I = 0; I < SCDesc->NumWriteLatencyEntries; ++I) {
    const auto* const Entry = SubtargetInfo->getWriteLatencyEntry(SCDesc, I);
    if (Entry->Cycles > MaxDefLatency) {
      MaxDefLatency = Entry->Cycles;
    }
  }

#if !defined(NDEBUG)
  if (MaxDefLatency < Uops->size()) {
    // These are TD mistakes that should be fixed in LLVM.
    llvm::errs() << InstrInfo->getName(Inst.getOpcode())
                 << ": inconsistent latency of " << MaxDefLatency << " for "
                 << Uops->size() << " uops\n";
  }
#endif

  // For lack of better information we distribute latency uniformly between
  // uops. Ideally this would be modeled by LLVM.
  // For example, if we have `MaxDefLatency = 5` and 3 uops, we'll assign 2
  // cycles to uops 0 and 1 and 1 cycle to uop 2.
  unsigned PrevEndCycle = 0;
  int RemainingUops = Uops->size();
  for (auto& Uop : *Uops) {
    Uop.StartCycle = PrevEndCycle;
    const int Latency =
        (MaxDefLatency + (RemainingUops - 1)) / RemainingUops;  // Round up.
    Uop.EndCycle = Uop.StartCycle + Latency;
    // NOTE(ondrasej): All uops must have latency at least 1, otherwise the
    // simulator will crash later. If an instruction has more uops than overall
    // latency, this loop will assign zero latency to at least one uop. We fix
    // this by making its latency one and preserving the end cycle, so that the
    // overall latency of the instruction is respected.
    if (Uop.EndCycle == Uop.StartCycle) {
      llvm::errs() << InstrInfo->getName(Inst.getOpcode()) << ": uop "
                   << (Uops->size() - RemainingUops)
                   << " has zero latency, fixing it to one.";
      assert(Uop.StartCycle > 0);
      Uop.StartCycle = Uop.EndCycle - 1;
    }
    MaxDefLatency -= Latency;
    --RemainingUops;
    PrevEndCycle = Uop.EndCycle;
  }
}

const InstrUopDecomposition& GlobalContext::GetInstructionDecomposition(
    const llvm::MCInst& Inst) const {
  auto& Result = DecompositionCache_[Inst];
  if (Result) {
    return *Result;
  }
  // The decomposition is not cached; compute it.
  if (!ResourceHierarchy_) {
    assert(SchedModel && "instruction decomposition requires a SchedModel");
    ResourceHierarchy_ = absl::make_unique<ResourceHierarchy>(*SchedModel);
  }

  Result = absl::make_unique<InstrUopDecomposition>();
  ComputeInstructionUops(Inst, &Result->Uops);
  ComputeUopLatencies(Inst, &Result->Uops);
  return *Result;
}

BlockContext::BlockContext(llvm::ArrayRef<llvm::MCInst> Instructions,
                           bool IsLoop)
    : Instructions_(Instructions), IsLoop_(IsLoop) {}

bool GlobalContext::MCInstEq::operator()(const llvm::MCInst& A,
                                         const llvm::MCInst& B) const {
  if (A.getOpcode() != B.getOpcode()) {
    return false;
  }
  if (A.getFlags() != B.getFlags()) {
    return false;
  }
  if (A.getNumOperands() != B.getNumOperands()) {
    return false;
  }
  const auto OpEq = [](const llvm::MCOperand& OpA, const llvm::MCOperand& OpB) {
    if (!OpA.isValid()) {
      return !OpB.isValid();
    }
    if (OpA.isReg()) {
      return OpB.isReg() && OpA.getReg() == OpB.getReg();
    }
    if (OpA.isImm()) {
      return OpB.isImm() && OpA.getImm() == OpB.getImm();
    }
    if (OpA.isDFPImm()) {
      return OpB.isDFPImm() && OpA.getDFPImm() == OpB.getDFPImm();
    }
    return true;
  };
  for (int I = 0; I < A.getNumOperands(); ++I) {
    if (!OpEq(A.getOperand(I), B.getOperand(I))) {
      return false;
    }
  }
  return true;
}

std::unique_ptr<GlobalContext> CreateGlobalContextForClif(
    const std::string& LlvmTriple, const std::string& CpuName) {
  return GlobalContext::CreateMutable(LlvmTriple, CpuName);
}

}  // namespace simulator
}  // namespace exegesis
