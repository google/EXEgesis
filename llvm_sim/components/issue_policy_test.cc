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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace simulator {
namespace {

using testing::ElementsAre;

TEST(IssuePolicyTest, Greedy) {
  const auto Policy = IssuePolicy::Greedy();
  std::vector<size_t> Ports = {2, 1, 3};
  Policy->ComputeBestOrder(Ports);
  ASSERT_THAT(Ports, ElementsAre(2, 1, 3));
}

TEST(IssuePolicyTest, LeastLoaded) {
  const auto Policy = IssuePolicy::LeastLoaded();

  // No load yet, same as greedy.
  {
    std::vector<size_t> Ports = {2, 1, 3};
    Policy->ComputeBestOrder(Ports);
    ASSERT_THAT(Ports, ElementsAre(2, 1, 3));
  }

  // Load port 1.
  {
    std::vector<size_t> Ports = {2, 1, 3};
    Policy->SignalIssued(2);
    Policy->ComputeBestOrder(Ports);
    ASSERT_THAT(Ports, ElementsAre(1, 3, 2));
  }

  {
    std::vector<size_t> Ports = {2, 1, 3};
    Policy->Reset();
    // No load yet, same as greedy.
    Policy->ComputeBestOrder(Ports);
    ASSERT_THAT(Ports, ElementsAre(2, 1, 3));
  }
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis
