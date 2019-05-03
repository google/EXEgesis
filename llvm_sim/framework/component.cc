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

#include "llvm_sim/framework/component.h"

namespace exegesis {
namespace simulator {

Component::Component(const GlobalContext* const Context) : Context(*Context) {}

Component::~Component() {}

Buffer::~Buffer() {}

Logger::~Logger() {}

std::string InstructionIndex::Format(const Type& Elem) {
  return llvm::Twine(Elem.Iteration)
      .concat(",")
      .concat(llvm::Twine(Elem.BBIndex))
      .str();
}
const char InstructionIndex::kTagName[] = "InstructionIndex";

bool InstructionIndex::Consume(llvm::StringRef& Input, Type& Elem) {
  unsigned long long Iteration;  // NOLINT
  if (llvm::consumeUnsignedInteger(Input, /*Radix=*/10, Iteration)) {
    return true;
  }
  if (Input.empty() || Input.front() != ',') {
    return true;
  }
  Input = Input.drop_front();
  unsigned long long BBIndex;  // NOLINT
  if (llvm::consumeUnsignedInteger(Input, /*Radix=*/10, BBIndex)) {
    return true;
  }
  Elem.Iteration = Iteration;
  Elem.BBIndex = BBIndex;
  return false;
}

std::string UopId::Format(const Type& Elem) {
  return llvm::Twine(InstructionIndex::Format(Elem.InstrIndex))
      .concat(",")
      .concat(llvm::Twine(Elem.UopIndex))
      .str();
}
const char UopId::kTagName[] = "UopId";

bool UopId::Consume(llvm::StringRef& Input, Type& Elem) {
  if (InstructionIndex::Consume(Input, Elem.InstrIndex)) {
    return true;
  }
  if (Input.empty() || Input.front() != ',') {
    return true;
  }
  Input = Input.drop_front();
  unsigned long long UopIndex;  // NOLINT
  if (llvm::consumeUnsignedInteger(Input, /*Radix=*/10, UopIndex)) {
    return true;
  }
  Elem.UopIndex = UopIndex;
  return false;
}

}  // namespace simulator
}  // namespace exegesis
