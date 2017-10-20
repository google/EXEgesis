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

#include "base/stringprintf.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

using ::exegesis::util::StatusOr;

StatusOr<std::vector<uint8_t>> ParseHexString(const std::string& hex_string) {
  ::re2::

      StringPiece hex_stringpiece(hex_string);
  std::vector<uint8_t> bytes;
  uint32_t encoded_byte;
  const RE2 byte_parser("(?:0x)?([0-9a-fA-F]{1,2}) *,? *");
  while (!hex_stringpiece.empty() &&
         RE2::Consume(&hex_stringpiece, byte_parser, RE2::Hex(&encoded_byte))) {
    bytes.push_back(static_cast<uint8_t>(encoded_byte));
  }
  if (!hex_stringpiece.empty()) {
    return util::InvalidArgumentError(
        StrCat("Could not parse: ", hex_stringpiece.ToString()));
  }
  return bytes;
}

}  // namespace exegesis
