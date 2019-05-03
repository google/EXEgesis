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

#include "llvm_sim/components/common.h"

#include <string>

namespace exegesis {
namespace simulator {

std::string RenamedUopId::Format(const Type& Elem) {
  return UopId::Format(Elem.Uop);
}
const char RenamedUopId::kTagName[] = "UopId";

}  // namespace simulator
}  // namespace exegesis
