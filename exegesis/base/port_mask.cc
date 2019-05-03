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

#include "exegesis/base/port_mask.h"

#include "absl/strings/str_cat.h"

namespace exegesis {

PortMask::PortMask(const PortMaskProto& proto) {
  uint64_t result = 0;
  for (auto port_number : proto.port_numbers()) {
    CHECK_GE(port_number, 0);
    CHECK_LT(port_number, 8);
    result |= (1ULL << port_number);
  }
  mask_ = result;
}

std::string PortMask::ToString() const {
  if (mask_ == 0) return "";
  std::string result = "P";
  uint64_t mask = mask_;
  for (int i = 0; mask != 0; ++i) {
    if (mask & (uint64_t{1} << i)) {
      mask -= (uint64_t{1} << i);
      absl::StrAppend(&result, i);
    }
  }
  return result;
}

PortMaskProto PortMask::ToProto() const {
  PortMaskProto proto;
  uint64_t mask = mask_;
  for (int i = 0; mask != 0; ++i) {
    if (mask & (uint64_t{1} << i)) {
      mask -= (uint64_t{1} << i);
      proto.add_port_numbers(i);
    }
  }
  return proto;
}

}  // namespace exegesis
