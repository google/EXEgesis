// Copyright 2016 Google Inc.
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

#ifndef EXEGESIS_BASE_CATEGORY_UTIL_H_
#define EXEGESIS_BASE_CATEGORY_UTIL_H_

#include <cstdint>

namespace exegesis {

// Returns true if the given value is in the given category. Both category and
// value are integral types. Both are interpreted as sequences of 4-bit numbers.
// We say that a value belongs to a category if its sequence is a prefix of the
// sequence of the value.
//
// Examples:
// 0x1234 belongs to potential categories 0x0, 0x1, 0x12, 0x123 and 0x1234, but
// not to potential category 0x2. Note that by this definition, all values
// belong to category 0.
inline bool InCategory(int32_t value, int32_t category) {
  while (value > category) {
    value = value >> 4;
  }
  return value == category;
}

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CATEGORY_UTIL_H_
