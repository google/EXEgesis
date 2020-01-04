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

#include "exegesis/base/opcode.h"

#include "absl/strings/str_format.h"

namespace exegesis {

std::string Opcode::ToString() const {
  std::string buffer;
  if (value_ > 0xffffff) {
    absl::StrAppendFormat(&buffer, "%02X ", value_ >> 24);
  }
  if (value_ > 0xffff) {
    absl::StrAppendFormat(&buffer, "%02X ", 0xff & (value_ >> 16));
  }
  if (value_ > 0xff) {
    absl::StrAppendFormat(&buffer, "%02X ", 0xff & (value_ >> 8));
  }
  absl::StrAppendFormat(&buffer, "%02X", 0xff & value_);
  return buffer;
}

std::ostream& operator<<(std::ostream& os, const exegesis::Opcode& opcode) {
  os << opcode.ToString();
  return os;
}

}  // namespace exegesis
