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

// A Port is simply a buffer of one element that can dispatch to a bunch of
// execution units. The buffer is cleared whenever an execution unit pops its
// only element.
#ifndef EXEGESIS_LLVM_SIM_COMPONENTS_PORT_H_
#define EXEGESIS_LLVM_SIM_COMPONENTS_PORT_H_

#include <string>

#include "llvm/ADT/Optional.h"
#include "llvm_sim/components/buffer.h"
#include "llvm_sim/components/common.h"
#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

// See top comment.
template <typename ElemTag>
class IssuePort : public LinkBuffer<ElemTag> {
 public:
  IssuePort() : LinkBuffer<ElemTag>(1) {}

  ~IssuePort() override {}
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_COMPONENTS_PORT_H_
