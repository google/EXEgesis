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

#include "exegesis/base/restrict.h"

#include <strings.h>

#include <string>

#include "absl/strings/ascii.h"
#include "exegesis/util/instruction_syntax.h"
#include "glog/logging.h"
#include "net/proto2/util/public/repeated_field_util.h"

namespace exegesis {
namespace {

// TODO(ondrasej): Replace this with an appropriate function from Abseil, once
// there is one.
int StringCaseCompare(const std::string& left, const std::string& right) {
  return strcasecmp(left.c_str(), right.c_str());
}

const std::string& GetLexicographicallyFirstMnemonicOrDie(
    const InstructionProto& instruction) {
  CHECK_GT(instruction.vendor_syntax_size(), 0);
  const std::string* first_mnemonic = &instruction.vendor_syntax(0).mnemonic();
  for (int i = 1; i < instruction.vendor_syntax_size(); ++i) {
    const std::string& mnemonic = instruction.vendor_syntax(i).mnemonic();
    if (StringCaseCompare(mnemonic, *first_mnemonic) < 0) {
      first_mnemonic = &mnemonic;
    }
  }
  return *first_mnemonic;
}

}  // namespace

void RestrictToMnemonicRange(const std::string& first_mnemonic,
                             const std::string& last_mnemonic,
                             InstructionSetProto* instruction_set) {
  RemoveIf(
      instruction_set->mutable_instructions(),
      [first_mnemonic, last_mnemonic](const InstructionProto* instruction) {
        const std::string& mnemonic =
            GetLexicographicallyFirstMnemonicOrDie(*instruction);
        return StringCaseCompare(mnemonic, first_mnemonic) < 0 ||
               StringCaseCompare(mnemonic, last_mnemonic) > 0;
      });
}

void RestrictToIndexRange(size_t start_index, size_t end_index,
                          InstructionSetProto* instruction_set,
                          InstructionSetItinerariesProto* itineraries) {
  CHECK_LE(start_index, end_index);
  // Remove all instructions after the end.
  while (instruction_set->instructions_size() > end_index) {
    instruction_set->mutable_instructions()->RemoveLast();
  }
  // Remove all instructions before the beginning.
  if (start_index < instruction_set->instructions_size()) {
    instruction_set->mutable_instructions()->DeleteSubrange(0, start_index);
  } else {
    instruction_set->clear_instructions();
  }
  if (itineraries) {
    while (itineraries->itineraries_size() > end_index) {
      itineraries->mutable_itineraries()->RemoveLast();
    }
    if (start_index < itineraries->itineraries_size()) {
      itineraries->mutable_itineraries()->DeleteSubrange(0, start_index);
    } else {
      itineraries->clear_itineraries();
    }
  }
}

}  // namespace exegesis
