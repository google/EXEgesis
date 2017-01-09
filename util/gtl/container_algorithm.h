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

#ifndef UTIL_GTL_CONTAINER_ALGORITHM_H_
#define UTIL_GTL_CONTAINER_ALGORITHM_H_

#include <algorithm>
#include <iterator>

namespace cpu_instructions {
namespace gtl {

// Returns true if 'container' contains an element equal to value.
template <typename Container, typename ValueType>
bool c_linear_search(const Container& container, const ValueType& value) {
  return std::find(std::begin(container), std::end(container), value) !=
         std::end(container);
}

}  // namespace gtl
}  // namespace cpu_instructions

#endif  // UTIL_GTL_CONTAINER_ALGORITHM_H_
