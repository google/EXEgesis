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

#include "llvm_sim/components/reorder_buffer.h"

#include <numeric>
#include <sstream>

namespace exegesis {
namespace simulator {

const char ROBUopId::kTagName[] = "UopId";
std::string ROBUopId::Format(const Type& Elem) {
  return UopId::Format(Elem.Uop);
}

ReorderBuffer::Buffer::Buffer(size_t Size) : Entries_(Size) {
  assert(Size > 0);
  Reset();
}

void ReorderBuffer::Buffer::Reset() {
  // Mark all entries as empty.
  const size_t Size = Entries_.size();
  Entries_.clear();
  Entries_.resize(Size);
  for (size_t I = 0; I < Size; ++I) {
    Entries_[I].ROBUop.ROBEntryIndex = I;
  }
  FirstEmptyEntryIndex_ = 0;
  NumEmptyEntries_ = Entries_.size();
  FirstRetirableEntryIndex_ = 0;
}

ReorderBuffer::ROBEntry& ReorderBuffer::Buffer::operator[](size_t Index) {
  if (Index == -1) {
    return Entries_.back();
  }
  if (Index == Entries_.size()) {
    return Entries_.front();
  }
  return Entries_[Index];
}

const ReorderBuffer::ROBEntry& ReorderBuffer::Buffer::operator[](
    size_t Index) const {
  if (Index == -1) {
    return Entries_.back();
  }
  if (Index == Entries_.size()) {
    return Entries_.front();
  }
  return Entries_[Index];
}

ReorderBuffer::ROBEntry* ReorderBuffer::Buffer::ReserveEntry() {
  if (NumEmptyEntries_ == 0) {
    return nullptr;
  }
  assert(Entries_[FirstEmptyEntryIndex_].State == ROBEntry::StateE::kEmpty);
  ROBEntry* const Result = &Entries_[FirstEmptyEntryIndex_];
  assert(NumEmptyEntries_ > 0);
  --NumEmptyEntries_;
  if (++FirstEmptyEntryIndex_ == Entries_.size()) {
    FirstEmptyEntryIndex_ = 0;
  }
  return Result;
}

void ReorderBuffer::ROBEntry::Clear() {
  State = ROBEntry::StateE::kEmpty;
  UnsatisfiedDependencies.empty();
  Defs.clear();
  PossiblePorts.clear();
  DependentEntries.clear();
}

void ReorderBuffer::Buffer::ReleaseOldestEntry() {
  const size_t Index = GetOldestEntryIndex();
  assert(Index < Entries_.size());
  assert(Entries_[Index].State == ROBEntry::StateE::kRetired);
  Entries_[Index].Clear();
  ++NumEmptyEntries_;
}

size_t ReorderBuffer::Buffer::GetOldestEntryIndex() const {
  size_t Index = FirstEmptyEntryIndex_ + NumEmptyEntries_;
  if (Index >= Entries_.size()) {
    Index -= Entries_.size();
  }
  return Index;
}

const ReorderBuffer::ROBEntry* ReorderBuffer::Buffer::GetRetirableEntry()
    const {
  const ROBEntry& Entry = Entries_[FirstRetirableEntryIndex_];
  return Entry.State == ROBEntry::StateE::kReadyToRetire ? &Entry : nullptr;
}

void ReorderBuffer::Buffer::PopRetirableEntry() {
  ROBEntry& Entry = Entries_[FirstRetirableEntryIndex_];
  assert(Entry.State == ROBEntry::StateE::kReadyToRetire);
  Entry.State = ROBEntry::StateE::kSentForRetirement;
  if (++FirstRetirableEntryIndex_ == Entries_.size()) {
    FirstRetirableEntryIndex_ = 0;
  }
}

ReorderBuffer::ReorderBuffer(
    const GlobalContext* Context, const Config& Config,
    Source<RenamedUopId>* UopSource, Source<ROBUopId>* AvailableDepsSource,
    Source<ROBUopId>* WritebackSource, Source<ROBUopId>* RetiredSource,
    Sink<ROBUopId>* IssuedSink, std::vector<Sink<ROBUopId>*> PortSinks,
    Sink<ROBUopId>* RetirementSink, std::unique_ptr<IssuePolicy> IssuePolicy)
    : Component(Context),
      Config_(Config),
      UopSource_(UopSource),
      WritebackSource_(WritebackSource),
      RetiredSource_(RetiredSource),
      IssuedSink_(IssuedSink),
      AvailableDepsSource_(AvailableDepsSource),
      PortSinks_(std::move(PortSinks)),
      RetirementSink_(RetirementSink),
      IssuePolicy_(std::move(IssuePolicy)),
      Entries_(Config.NumROBEntries) {}

ReorderBuffer::~ReorderBuffer() {}

void ReorderBuffer::Init() {
  Entries_.Reset();
  IssuePolicy_->Reset();
  InFlightRegisterDefs_.clear();
}

void ReorderBuffer::Tick(const BlockContext* BlockContext) {
  // Free entries for the uops that were retired by the Reservation Station
  // during the previous cycle. This cannot stall and happens before all other
  // stages.
  DeleteRetiredUops();

  // Read uops from the source. This can only add new entries.
  ReadNewUops(BlockContext);

  // Update uop dependencies to reflect which dependencies are going to be made
  // available next cycle.
  UpdateDataDependencies();

  // Update the uops that have finished executing on execution units in the
  // previous cycle. This cannot stall and happens before
  // SendUopsForRetirement(). This does not add or delete any entries.
  UpdateWrittenBackUops();

  // Send ready-to-execute uops to issue ports. This might stall if no ports are
  // available to issue the uop, in which case the uop remains kReadyToExecute,
  // and another uop is tried. This does not add or delete any entries.
  SendUopsForExecution();

  // Send uops that are ready for retirement. This might stall, in which case
  // the remaining uops simply remain kReadyToRetire. This does not add or
  // delete any entries.
  SendUopsForRetirement();
}

void ReorderBuffer::SendUopsForRetirement() {
  // Now send the uops to the retirement sink.
  while (const auto* Entry = Entries_.GetRetirableEntry()) {
    if (!RetirementSink_->Push(Entry->ROBUop)) {
      return;
    }
    Entries_.PopRetirableEntry();
  }
}

void ReorderBuffer::SendUopsForExecution() {
  for (auto& Entry : Entries_) {
    if (Entry.State == ROBEntry::StateE::kReadyToExecute) {
      auto OrderedPorts = Entry.PossiblePorts;
      if (OrderedPorts.empty()) {
        // The uop does not use an execution unit.
        Entry.State = ROBEntry::StateE::kReadyToRetire;
        UpdateDependentEntries(Entry);
      } else {
        // Try pushing on the best possible ports until one is available.
        IssuePolicy_->ComputeBestOrder(OrderedPorts);
        for (const size_t Port : OrderedPorts) {
          if (PortSinks_[Port]->Push(Entry.ROBUop)) {
            IssuePolicy_->SignalIssued(Port);
            const bool IssuedPush = IssuedSink_->Push(Entry.ROBUop);
            assert(IssuedPush);
            (void)IssuedPush;
            Entry.State = ROBEntry::StateE::kIssued;
            break;
          }
        }
      }
    }
  }
}

void ReorderBuffer::UpdateDataDependencies() {
  while (const ROBUopId::Type* AvailableDeps = AvailableDepsSource_->Peek()) {
    auto& Entry = Entries_[AvailableDeps->ROBEntryIndex];
    assert(Entry.State == ROBEntry::StateE::kIssued);
    Entry.State = ROBEntry::StateE::kOutputsAvailableNextCycle;
    // TODO(courbet): Model writeback delays.
    UpdateDependentEntries(Entry);
    AvailableDepsSource_->Pop();
  }
}

void ReorderBuffer::UpdateWrittenBackUops() {
  while (const ROBUopId::Type* WrittenBack = WritebackSource_->Peek()) {
    auto& Entry = Entries_[WrittenBack->ROBEntryIndex];
    assert(Entry.State == ROBEntry::StateE::kOutputsAvailableNextCycle);
    Entry.State = ROBEntry::StateE::kReadyToRetire;
    WritebackSource_->Pop();
  }
}

// Update the state of dependent uops.
void ReorderBuffer::UpdateDependentEntries(const ROBEntry& Entry) {
  for (const size_t DepIndex : Entry.DependentEntries) {
    auto& Dep = Entries_[DepIndex];
    assert(Dep.State == ROBEntry::StateE::kWaitingForInputs);
    assert(Dep.UnsatisfiedDependencies.count(Entry.ROBUop.ROBEntryIndex));
    // Mark this dep as satisfied.
    Dep.UnsatisfiedDependencies.erase(Entry.ROBUop.ROBEntryIndex);
    // If there are no remaining deps, the entry becomes ready for execution.
    if (Dep.UnsatisfiedDependencies.empty()) {
      Dep.State = ROBEntry::StateE::kReadyToExecute;
    }
  }
}

void ReorderBuffer::DeleteRetiredUops() {
  while (const ROBUopId::Type* Retired = RetiredSource_->Peek()) {
    Entries_[Retired->ROBEntryIndex].State = ROBEntry::StateE::kRetired;
    // When uops retire, the register defs are removed from the "in flight" list
    // (in the real CPU, the values for these registers get committed to the
    // Register File).
    for (const size_t Def : Entries_[Retired->ROBEntryIndex].Defs) {
      InFlightRegisterDefs_.erase(Def);
    }
    // Release the entry.
    Entries_.ReleaseOldestEntry();
    RetiredSource_->Pop();
  }
}

void ReorderBuffer::ReadNewUops(const BlockContext* BlockContext) {
  while (const RenamedUopId::Type* Uop = UopSource_->Peek()) {
    ROBEntry* Entry = Entries_.ReserveEntry();
    if (!Entry) {
      return;  // No more free entries.
    }
    Entry->State = ROBEntry::StateE::kWaitingForInputs;
    Entry->ROBUop.Uop = Uop->Uop;
    Entry->Defs = Uop->Defs;
    SetPossiblePortsAndLatencies(BlockContext, Entry);
    SetInputDependencies(BlockContext, Uop->Uses, Entry);
    for (const size_t Def : Uop->Defs) {
      InFlightRegisterDefs_.emplace(Def, Entry->ROBUop.ROBEntryIndex);
    }
    if (Entry->UnsatisfiedDependencies.empty()) {
      Entry->State = ROBEntry::StateE::kReadyToExecute;
    }
    UopSource_->Pop();
  }
}

void ReorderBuffer::SetPossiblePortsAndLatencies(
    const BlockContext* BlockContext, ROBEntry* const Entry) const {
  const auto InstrIndex = Entry->ROBUop.Uop.InstrIndex;
  const auto& Uop = Context
                        .GetInstructionDecomposition(
                            BlockContext->GetInstruction(InstrIndex.BBIndex))
                        .Uops[Entry->ROBUop.Uop.UopIndex];

  Entry->ROBUop.Latency = Uop.GetLatency();
  if (Uop.ProcResIdx == 0) {
    // This uop does not consume any proc resources (e.g. register-to-register
    // move).
    return;
  }
  const llvm::MCProcResourceDesc* const ProcResDesc =
      Context.SchedModel->getProcResource(Uop.ProcResIdx);

  if (ProcResDesc->SubUnitsIdxBegin == nullptr) {
    // This resource is a unit resource.
    Entry->PossiblePorts.push_back(Uop.ProcResIdx - 1);
  } else {
    // This resource is a ProcResGroup, dispatch to any of the underlying unit
    // resources.
    for (const auto* SubResIdx = ProcResDesc->SubUnitsIdxBegin;
         SubResIdx != ProcResDesc->SubUnitsIdxBegin + ProcResDesc->NumUnits;
         ++SubResIdx) {
      Entry->PossiblePorts.push_back(*SubResIdx - 1);
    }
  }
}

void ReorderBuffer::SetInputDependencies(const BlockContext* BlockContext,
                                         llvm::ArrayRef<size_t> Uses,
                                         ROBEntry* const Entry) {
  for (const size_t Use : Uses) {
    const auto DefinerIt = InFlightRegisterDefs_.find(Use);
    // Case 1: The register has last been modified by a µop that has been
    // retired. Read from the Register File. TODO(courbet): Some older CPUs
    // were incurring a delay here. Model this.
    if (DefinerIt == InFlightRegisterDefs_.end()) {
      continue;
    }
    // Case 2: The register has last been modified by a µop that has been
    // executed but not yet retired. The data is already available, there is
    // no need to create a dependency.
    ROBEntry& DeferEntry = Entries_[DefinerIt->second];
    if (DeferEntry.State == ROBEntry::StateE::kOutputsAvailableNextCycle ||
        DeferEntry.State == ROBEntry::StateE::kReadyToRetire ||
        DeferEntry.State == ROBEntry::StateE::kSentForRetirement) {
      continue;
    }
    // Case 3: The register will be modified by a µop that is not yet done
    // executing, we need to create a dependency. It's possible that a µop
    // modifies more than one register we depend on; when this happens, we add
    // only one dependency. Otherwise, removing the µop index from
    // DependentEntries and setting its state to kReadyToExecute would result in
    // DependentEntries unexpectedly containing an index of a kReadyToExecute
    // µop.
    const size_t DeferEntryROBIndex = DeferEntry.ROBUop.ROBEntryIndex;
    if (Entry->UnsatisfiedDependencies.count(DeferEntryROBIndex) == 0) {
      Entry->UnsatisfiedDependencies.insert(DeferEntryROBIndex);
      DeferEntry.DependentEntries.push_back(Entry->ROBUop.ROBEntryIndex);
    }
  }
  // TODO(courbet): Better model intra-instruction uop dependencies. Right now
  // we assume that each uop depends on the previous one.
  if (Entry->ROBUop.Uop.UopIndex != 0) {
    auto& PrevEntry = Entries_[Entry->ROBUop.ROBEntryIndex - 1];
    assert(PrevEntry.ROBUop.Uop.UopIndex == Entry->ROBUop.Uop.UopIndex - 1);
    // Only add the dependency if the uop we're depending on is not already done
    // executing.
    if (!(PrevEntry.State == ROBEntry::StateE::kOutputsAvailableNextCycle ||
          PrevEntry.State == ROBEntry::StateE::kReadyToRetire ||
          PrevEntry.State == ROBEntry::StateE::kSentForRetirement)) {
      Entry->UnsatisfiedDependencies.insert(PrevEntry.ROBUop.ROBEntryIndex);
      PrevEntry.DependentEntries.push_back(Entry->ROBUop.ROBEntryIndex);
    }
  }
}

template <typename BufferT, typename ElemT>
ReorderBuffer::Buffer::IteratorBase<BufferT, ElemT>::IteratorBase(
    BufferT* Container, size_t Index)
    : Container_(Container), Index_(Index) {}

template <typename BufferT, typename ElemT>
ElemT& ReorderBuffer::Buffer::IteratorBase<BufferT, ElemT>::operator*() const {
  size_t Index = Index_;
  if (Index_ >= Container_->Size()) {
    Index -= Container_->Size();
  }
  return (*Container_)[Index];
}

template <typename BufferT, typename ElemT>
ReorderBuffer::Buffer::IteratorBase<BufferT, ElemT>&
ReorderBuffer::Buffer::IteratorBase<BufferT, ElemT>::operator++() {
  if (Index_ < 2 * Container_->Size() - 1) {
    ++Index_;
  }
  return *this;
}

template <typename BufferT, typename ElemT>
bool ReorderBuffer::Buffer::IteratorBase<BufferT, ElemT>::operator!=(
    const IteratorBase<BufferT, ElemT>& other) const {
  assert(Container_ == other.Container_ && "incompatible containers");
  return Index_ != other.Index_;
}

ReorderBuffer::Buffer::iterator ReorderBuffer::Buffer::begin() {
  return iterator(this, GetOldestEntryIndex());
}

ReorderBuffer::Buffer::const_iterator ReorderBuffer::Buffer::begin() const {
  return const_iterator(this, GetOldestEntryIndex());
}

ReorderBuffer::Buffer::iterator ReorderBuffer::Buffer::end() {
  size_t Index = FirstEmptyEntryIndex_;
  if (NumEmptyEntries_ < Size() &&
      GetOldestEntryIndex() >= FirstEmptyEntryIndex_) {
    Index += Size();
  }
  return iterator(this, Index);
}

ReorderBuffer::Buffer::const_iterator ReorderBuffer::Buffer::end() const {
  size_t Index = FirstEmptyEntryIndex_;
  if (NumEmptyEntries_ < Size() &&
      GetOldestEntryIndex() >= FirstEmptyEntryIndex_) {
    Index += Size();
  }
  return const_iterator(this, Index);
}

void ReorderBuffer::ROBEntry::DebugPrint(llvm::raw_ostream& OS) const {
  OS << ROBUop.ROBEntryIndex << ": ";
  switch (State) {
    case StateE::kEmpty:
      OS << "  State: kEmpty\n";
      break;
    case StateE::kWaitingForInputs:
      OS << "  State: kWaitingForInputs\n";
      break;
    case StateE::kReadyToExecute:
      OS << "  State: kReadyToExecute\n";
      break;
    case StateE::kIssued:
      OS << "  State: kIssued\n";
      break;
    case StateE::kOutputsAvailableNextCycle:
      OS << "  State: kOutputsAvailableNextCycle\n";
      break;
    case StateE::kReadyToRetire:
      OS << "  State: kReadyToRetire\n";
      break;
    case StateE::kSentForRetirement:
      OS << "  State: kSentForRetirement\n";
      break;
    case StateE::kRetired:
      OS << "  State: kRetired\n";
      break;
  }
  if (State != StateE::kEmpty) {
    OS << "  Uop: {Iteration:" << ROBUop.Uop.InstrIndex.Iteration
       << ", BBIndex:" << ROBUop.Uop.InstrIndex.BBIndex
       << ", UopIndex:" << ROBUop.Uop.UopIndex << "}\n";
    OS << "  DependentEntries:";
    for (const auto Dep : DependentEntries) {
      OS << " " << Dep;
    }
    OS << "\n";
  }
  switch (State) {
    case StateE::kWaitingForInputs:
      OS << "  UnsatisfiedDeps:";
      for (const auto Dep : UnsatisfiedDependencies) {
        OS << " " << Dep;
      }
      OS << "\n";
      break;
    case StateE::kReadyToExecute:
      OS << "  PossiblePorts:";
      for (const auto Port : PossiblePorts) {
        OS << " " << Port;
      }
      OS << "\n";
      break;
    default:
      break;
  }
}

std::string ReorderBuffer::DebugString() const {
  std::string Result;
  llvm::raw_string_ostream OS(Result);
  // Start from the oldest entry.
  for (const auto& Entry : Entries_) {
    Entry.DebugPrint(OS);
  }
  OS.flush();
  return Result;
}

}  // namespace simulator
}  // namespace exegesis
