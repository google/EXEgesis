// Copyright 2017 Google Inc.
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

#ifndef EXEGESIS_BASE_HOST_CPU_H_
#define EXEGESIS_BASE_HOST_CPU_H_

#include <unordered_set>
#include "strings/string.h"

namespace cpu_instructions {

class HostCpuInfo {
 public:
  // Returns the CPU info for the host we're running on.
  static const HostCpuInfo& Get();

  HostCpuInfo(const string& id, std::unordered_set<string> indexed_features);

  // Returns the CPU model id (e.g. "intel:06_3F").
  const string& cpu_id() const { return cpu_id_; }

  // Returns true if the CPU supports this feature. See
  // cpu_instructions.InstructionProto.feature_name for the syntax.
  bool SupportsFeature(const string& feature_name) const;

  string DebugString() const;

 private:
  // Returns true if the host CPU suports a feature with this exact name.
  bool HasExactFeature(const string& name) const;

  // Returns true if the name is a set (conjunction or disjunction) of features
  // rather than a single feature.
  // Sets *value to true if the host CPU supports:
  //  - any of the features A, ..., C specified as "A || ... || C" if is_or is
  //    true.
  //  - all of the features A, ..., C specified as "A && ... && C" if
  //    is_or is false.
  template <bool is_or>
  bool IsFeatureSet(const string& name, bool* value) const;

  const string cpu_id_;
  const std::unordered_set<string> indexed_features_;
};

}  // namespace cpu_instructions

#endif  // EXEGESIS_BASE_HOST_CPU_H_
