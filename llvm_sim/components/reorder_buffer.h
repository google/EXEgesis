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

// The reorder buffer does the bookkeeping of uop states.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_REORDER_BUFFER_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_REORDER_BUFFER_H_

#include <set>

#include "absl/container/flat_hash_map.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/components/issue_policy.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// The id of a uop in the ROB.
struct ROBUopId {
  struct Type {
    size_t ROBEntryIndex;  // The index of the uop in the ROB.
    UopId::Type Uop;
    unsigned Latency;  // The execution latency for the uop.
  };

  static const char kTagName[];
  static std::string Format(const Type& Elem);
  static const InstructionIndex::Type& GetInstructionIndex(const Type& Elem) {
    return Elem.Uop.InstrIndex;
  }
};

class ReorderBuffer : public Component {
 public:
  struct Config {
    // The number of uops in the buffer.
    unsigned NumROBEntries;
  };

  ReorderBuffer(const GlobalContext* Context, const Config& Config,
                Source<RenamedUopId>* UopSource,
                Source<ROBUopId>* AvailableDepsSource,
                Source<ROBUopId>* WritebackSource,
                Source<ROBUopId>* RetiredSource, Sink<ROBUopId>* IssuedSink,
                std::vector<Sink<ROBUopId>*> PortSinks,
                Sink<ROBUopId>* RetirementSink,
                std::unique_ptr<IssuePolicy> IssuePolicy);

  ~ReorderBuffer() override;

  void Init() override;
  void Tick(const BlockContext* BlockContext) override;

  // Prints the state of the ROB.
  std::string DebugString() const;

 private:
  struct ROBEntry {
    enum class StateE {
      // There is no uop in the entry. The id of the entry is in EmptyEntries_.
      kEmpty,
      // The uop is waiting for its inputs tfo be ready.
      kWaitingForInputs,
      // All inputs are ready, the uop is ready to execute.
      kReadyToExecute,
      // The uop is sent to an issue port.
      kIssued,
      // The outputs of the uop are going to be ready for consumption next
      // cycle, dependent uops can be issued this cycle.
      kOutputsAvailableNextCycle,
      // The uop is done executing, it is ready to retire.
      kReadyToRetire,
      // The uop has been sent for retirement.
      kSentForRetirement,
      // The uop is fully retired.
      kRetired,
    };

    void Clear();
    void DebugPrint(llvm::raw_ostream& OS) const;

    StateE State = StateE::kEmpty;
    ROBUopId::Type ROBUop;
    // The list of microarchitectural registers def'ed by this uop.
    llvm::SmallVector<size_t, 8> Defs;
    // The list of PortSinks indices on which the Uop can schedule.
    llvm::SmallVector<size_t, 8> PossiblePorts;
    // The ROB entry indices on which this entry depends. The entry can be
    // dispatched only when these are done executing. Note that this does not
    // include uops that have already retired (the data for these has already
    // been written back to the register file).
    std::set<size_t> UnsatisfiedDependencies;
    // The ROB entries that depend on this entry.
    llvm::SmallVector<size_t, 8> DependentEntries;
  };

  // Read uops from the source and populate entries until we stall on any
  // resource (free ROB entries, register reads, ...).
  void ReadNewUops(const BlockContext* BlockContext);
  // Update dependencies of entries whose output is going to become available
  // next cycle.
  void UpdateDataDependencies();
  // Update entries that were written back.
  void UpdateWrittenBackUops();
  // Update the state of the entries that depend on `Entry`.
  void UpdateDependentEntries(const ROBEntry& Entry);
  // Delete entries corresponding to fully retired uops.
  void DeleteRetiredUops();

  // Sends uops for retirement. Note that entries stay in the ROB until they are
  // fully retired (i.e. they appear in RetiredSource_).
  void SendUopsForRetirement();
  // Sends uops to issue ports for execution. Entries stay in the ROB.
  void SendUopsForExecution();

  // Determines which ports the uop can be issued to.
  void SetPossiblePortsAndLatencies(const BlockContext* BlockContext,
                                    ROBEntry* const Entry) const;
  // Computes the input dependencies of the entry.
  void SetInputDependencies(const BlockContext* BlockContext,
                            llvm::ArrayRef<size_t> Uses, ROBEntry* const Entry);

  const Config Config_;

  Source<RenamedUopId>* const UopSource_;
  // The source of kReadyToRetire uops. Populated during execution writeback.
  Source<ROBUopId>* const WritebackSource_;
  // The source of kRetired uops. Populated by the retirement station.
  Source<ROBUopId>* const RetiredSource_;

  // The sink and source for resolving data dependency sequencing. The ROB
  // actually predicts when dependencies are going to be available. For example,
  // for back-to-back adds:
  //   add rax, rbx
  //   add rbx, rax
  // The first add will be issued at cycle `N`, and will execute and writeback
  // at cycle `N+1`. The ROB will know about the writeback only at cycle `N+2`.
  // However, the ROB will predict this and issue the second add at cycle `N+1`
  // so that it can execute and writeback at cycle `N+2`, when the value of rax
  // is already available.
  // Different architectures might have different approaches for this, so we
  // avoid putting this logic in the ROB itself: we delegate to an external
  // sink/source pair. Entries are pushed to the sink when they start executing
  // and become available on the source when their outputs are going to be
  // made available at the next cycle and dependent entries can thus be issued.
  // `IssuedSink_` should never stall (`Push()` must always return true).
  Sink<ROBUopId>* const IssuedSink_;
  Source<ROBUopId>* const AvailableDepsSource_;

  // The sinks to dispatch uops to issue ports: one sink per port
  // (a.k.a ProcResource in the scheduling model), indexed by
  // `ProcResourceIdx - 1`.
  const std::vector<Sink<ROBUopId>*> PortSinks_;
  // The sink to send uops for retirement.
  Sink<ROBUopId>* const RetirementSink_;

  // The policy for issuing uops to ports.
  const std::unique_ptr<IssuePolicy> IssuePolicy_;

  // The buffer. Note that because retirement happens in order, and entries
  // remain in the ROB until they are fully retired, this is a circular buffer.
  class Buffer {
   public:
    explicit Buffer(size_t Size);

    void Reset();
    size_t Size() const { return Entries_.size(); }

    // Subscript operators accept indices from `-1` to `Size()` so as to accept
    // previous and next element:
    // If `I` is a valid index, then both `Buffer[I-1]` and `Buffer[I+1]` are
    // valid.
    ROBEntry& operator[](size_t Index);
    const ROBEntry& operator[](size_t Index) const;

    // Releases the oldest entry, which must be in kRetired state.
    void ReleaseOldestEntry();

    // Returns the index of the oldest entry.
    size_t GetOldestEntryIndex() const;

    // Reserves the first free entry entry and returns it. Returns nullptr if
    // there are no empty entries.
    ROBEntry* ReserveEntry();

    // Returns the first retirable entry, or nullptr if there are no retirable
    // entries.
    const ROBEntry* GetRetirableEntry() const;

    // Pops the retirable entry.
    void PopRetirableEntry();

    // Range support. Iteration starts at the oldest entry and stops at the
    // youngest reserved one. Iterators are invalidated if entries are reserved
    // or released.
    template <typename BufferT, typename ElemT>
    class IteratorBase
        : public std::iterator<std::forward_iterator_tag, ElemT> {
     public:
      IteratorBase(BufferT* Container, size_t Index);
      ElemT& operator*() const;
      inline IteratorBase& operator++();
      inline bool operator!=(const IteratorBase& other) const;

     private:
      BufferT* const Container_;
      // The index is in [0;2*Container_.Size()-1].
      size_t Index_;
    };
    using iterator = IteratorBase<Buffer, ROBEntry>;
    using const_iterator = IteratorBase<const Buffer, const ROBEntry>;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

   private:
    std::vector<ROBEntry> Entries_;
    // The index of the first empty entry.
    size_t FirstEmptyEntryIndex_;
    // The number of empty entries.
    size_t NumEmptyEntries_;

    // The index of the first retirable entry, i.e. the entry that has all uops
    // before it already retired.
    size_t FirstRetirableEntryIndex_;
  };
  Buffer Entries_;

  // A map of microarchitectural register to the last live (not retired) entry
  // index that defs it.
  absl::flat_hash_map<size_t, size_t> InFlightRegisterDefs_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_REORDER_BUFFER_H_
