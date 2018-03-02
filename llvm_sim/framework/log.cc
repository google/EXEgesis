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

#include "llvm_sim/framework/log.h"

#include <iomanip>
#include <sstream>

namespace exegesis {
namespace simulator {

SimulationLog::SimulationLog(
    const std::vector<BufferDescription>& BufferDescriptions)
    : BufferDescriptions(BufferDescriptions) {}

std::string SimulationLog::DebugString() const {
  std::stringstream Out;
  auto SortedLines = Lines;
  std::sort(SortedLines.begin(), SortedLines.end(),
            [](const Line& A, const Line& B) {
              return std::tie(A.Cycle, A.BufferIndex, A.Msg) <
                     std::tie(B.Cycle, B.BufferIndex, B.Msg);
            });
  int CurCycle = -1;
  for (const auto& Line : SortedLines) {
    if (CurCycle != Line.Cycle) {
      CurCycle = Line.Cycle;
      Out << "\n--- Cycle " << CurCycle << " ---\n";
    }
    Out << "Buffer: \"" << BufferDescriptions[Line.BufferIndex].DisplayName
        << "\" (" << Line.BufferIndex << ")   MsgTag: \"" << Line.MsgTag
        << "\"   Msg: \"" << Line.Msg << "\"\n";
  }
  return Out.str();
}

size_t SimulationLog::GetNumCompleteIterations() const {
  return Iterations.size();
}

}  // namespace simulator
}  // namespace exegesis
