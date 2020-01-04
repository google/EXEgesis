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

#include "exegesis/util/strings.h"

#include "absl/strings/str_cat.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

using ::exegesis::util::StatusOr;

StatusOr<std::vector<uint8_t>> ParseHexString(absl::string_view hex_string) {
  std::vector<uint8_t> bytes;
  uint32_t encoded_byte;
  const RE2 byte_parser("(?:0x)?([0-9a-fA-F]{1,2}) *,? *");
  while (!hex_string.empty() &&
         RE2::Consume(&hex_string, byte_parser, RE2::Hex(&encoded_byte))) {
    bytes.push_back(static_cast<uint8_t>(encoded_byte));
  }
  if (!hex_string.empty()) {
    return util::InvalidArgumentError(
        absl::StrCat("Could not parse: ", hex_string));
  }
  return bytes;
}

void RemoveAllChars(std::string* text, absl::string_view chars) {
  DCHECK(text != nullptr) << "Input `text` cannot be nullptr";
  text->erase(std::remove_if(text->begin(), text->end(),
                             [chars](const char c) {
                               return chars.find(c) != absl::string_view::npos;
                             }),
              text->end());
}

void RemoveSpaceAndLF(std::string* text) {
  static constexpr absl::string_view kRemovedChars = "\n\r ";
  RemoveAllChars(text, kRemovedChars);
}

}  // namespace exegesis
