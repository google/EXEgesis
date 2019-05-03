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

// A class that defines how to compute which port a uop should be issued to.

#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_ISSUE_POLICY_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_ISSUE_POLICY_H_

#include "llvm/ADT/ArrayRef.h"

namespace exegesis {
namespace simulator {

class IssuePolicy {
 public:
  // A policy that picks the first available port.
  static std::unique_ptr<IssuePolicy> Greedy();

  // A policy that picks the least loaded port.
  static std::unique_ptr<IssuePolicy> LeastLoaded();

  virtual ~IssuePolicy();

  // Resets the state of the policy.
  virtual void Reset() = 0;

  // Signals that a uop has been issued on port I.
  virtual void SignalIssued(size_t I) = 0;

  // Orders the list of ports such that the preferred one comes first.
  virtual void ComputeBestOrder(
      const llvm::MutableArrayRef<size_t> PossiblePorts) = 0;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_ISSUE_POLICY_H_
