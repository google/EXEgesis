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

// Utilities to work on instruction sets.

#ifndef EXEGESIS_BASE_RESTRICT_H_
#define EXEGESIS_BASE_RESTRICT_H_

#include <string>

#include "exegesis/proto/instructions.pb.h"

namespace exegesis {

// Keeps only the instructions whose mnemonic is in the range
// [first_mnemonic, last_mnemonic].
void RestrictToMnemonicRange(const std::string& first_mnemonic,
                             const std::string& last_mnemonic,
                             InstructionSetProto* instruction_set);

// Keeps only the instructions whose index is in the range
// [start_index, end_index).
void RestrictToIndexRange(size_t start_index, size_t end_index,
                          InstructionSetProto* instruction_set,
                          InstructionSetItinerariesProto* itineraries);

}  // namespace exegesis

#endif  // EXEGESIS_BASE_RESTRICT_H_
