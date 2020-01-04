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

#include "llvm_sim/components/issue_policy.h"

#include "absl/memory/memory.h"

namespace exegesis {
namespace simulator {

IssuePolicy::~IssuePolicy() {}

namespace {

// No reordering: the best port is the first port (in `PossiblePort` order).
class GreedyIssuePolicy final : public IssuePolicy {
  void Reset() final {}
  void SignalIssued(size_t) final {}
  void ComputeBestOrder(const llvm::MutableArrayRef<size_t>) final {}
};

// Maintains port load and picks the least loaded port. Inspired by:
// https://stackoverflow.com/questions/40681331/how-are-x86-uops-scheduled-exactly
class LeastLoadedIssuePolicy final : public IssuePolicy {
  void Reset() final { PortLoads_.clear(); }

  void SignalIssued(size_t I) final {
    if (I >= PortLoads_.size()) {
      PortLoads_.resize(I + 1);
    }
    ++PortLoads_[I];
  }

  void ComputeBestOrder(
      const llvm::MutableArrayRef<size_t> PossiblePorts) final {
    std::stable_sort(
        PossiblePorts.begin(), PossiblePorts.end(),
        [this](size_t P, size_t Q) { return GetLoad(P) < GetLoad(Q); });
  }

 private:
  // Returns the load for port I.
  size_t GetLoad(size_t I) const {
    return I < PortLoads_.size() ? PortLoads_[I] : 0;
  }

  llvm::SmallVector<size_t, 8> PortLoads_;
};

}  // namespace

std::unique_ptr<IssuePolicy> IssuePolicy::Greedy() {
  return absl::make_unique<GreedyIssuePolicy>();
}

std::unique_ptr<IssuePolicy> IssuePolicy::LeastLoaded() {
  return absl::make_unique<LeastLoadedIssuePolicy>();
}

}  // namespace simulator
}  // namespace exegesis
