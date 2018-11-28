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

#include "exegesis/base/cpu_info.h"

#include <cstring>
#include <utility>

#include "absl/strings/str_cat.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {

bool CpuInfo::HasExactFeature(const std::string& name) const {
  return gtl::ContainsKey(indexed_features_, name);
}

// TODO(courbet): Right now absl::StrSplit() does not support a string literal
// delimiter. Switch to it when it does.
template <bool is_or>
bool CpuInfo::IsFeatureSet(const std::string& name, bool* const value) const {
  char kSeparator[5] = " && ";
  if (is_or) {
    kSeparator[1] = kSeparator[2] = '|';
  }
  const char* begin = name.data();
  const char* end = std::strstr(begin, kSeparator);
  if (end == nullptr) {
    return false;  // Not a feature set.
  }

  while (end != nullptr) {
    const bool has_feature = HasExactFeature(std::string(begin, end));
    if (is_or && has_feature) {
      *value = true;
      return true;
    }
    if (!is_or && !has_feature) {
      *value = false;
      return true;
    }
    begin = end + sizeof(kSeparator) - 1;
    end = std::strstr(begin, kSeparator);
  }
  *value = HasExactFeature(begin);
  return true;
}

bool CpuInfo::SupportsFeature(const std::string& feature_name) const {
  // We don't support parenthesized features combinations for now.
  CHECK(feature_name.find('(') == std::string::npos);
  CHECK(feature_name.find(')') == std::string::npos);

  bool value = false;
  if (IsFeatureSet<true>(feature_name, &value) ||
      IsFeatureSet<false>(feature_name, &value)) {
    return value;
  }
  return HasExactFeature(feature_name);
}

std::string CpuInfo::DebugString() const {
  std::string result = absl::StrCat(proto_.model_id(), "\nfeatures:");
  for (const std::string& feature : indexed_features_) {
    absl::StrAppend(&result, "\n", feature);
  }
  return result;
}
}  // namespace exegesis
