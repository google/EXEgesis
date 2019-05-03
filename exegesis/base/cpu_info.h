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

#ifndef EXEGESIS_BASE_CPU_INFO_H_
#define EXEGESIS_BASE_CPU_INFO_H_

#include <string>

#include "absl/container/flat_hash_set.h"
#include "exegesis/proto/microarchitecture.pb.h"

namespace exegesis {

// Contains the information about the CPU obtained from the CPUID (or
// equlivalent) instruction of the CPU. Provides access to the list of features
// supported by the CPU.
class CpuInfo {
 public:
  CpuInfo(const CpuInfoProto& proto)
      : proto_(proto),
        indexed_features_(proto_.feature_names().begin(),
                          proto_.feature_names().end()) {}

  // Returns the CPU model id (e.g. "intel:06_3F").
  const std::string& cpu_model_id() const { return proto_.model_id(); }

  // Returns true if the CPU supports this feature. See
  // exegesis.InstructionProto.feature_name for the syntax.
  bool SupportsFeature(const std::string& feature_name) const;

  const absl::flat_hash_set<std::string>& supported_features() const {
    return indexed_features_;
  }

  std::string DebugString() const;

  const CpuInfoProto& proto() const { return proto_; }

 private:
  // Returns true if the host CPU suports a feature with this exact name.
  bool HasExactFeature(const std::string& name) const;

  // Returns true if the name is a set (conjunction or disjunction) of features
  // rather than a single feature.
  // Sets *value to true if the host CPU supports:
  //  - any of the features A, ..., C specified as "A || ... || C" if is_or is
  //    true.
  //  - all of the features A, ..., C specified as "A && ... && C" if
  //    is_or is false.
  template <bool is_or>
  bool IsFeatureSet(const std::string& name, bool* value) const;

  const CpuInfoProto proto_;
  const absl::flat_hash_set<std::string> indexed_features_;
};

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CPU_INFO_H_
