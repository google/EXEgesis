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

#ifndef CPU_INSTRUCTIONS_UTIL_STRINGS_H_
#define CPU_INSTRUCTIONS_UTIL_STRINGS_H_

#include <cstdint>
#include <vector>
#include "strings/string.h"

#include "base/stringprintf.h"
#include "strings/string_view.h"
#include "util/task/statusor.h"

namespace cpu_instructions {

using ::cpu_instructions::util::StatusOr;

// Parses the given hexadecimal string in several possible formats:
// * each byte is encoded as one or two hexadecimal digits,
// * each byte can have an optional 0x prefix,
// * both uppercase and lowercase letters are accepted,
// * the bytes are separated either by spaces or by commas.
//
// Example input formats:
// * 0x0,0x1,0x2,0x3
// * 00 AB 01 BC
StatusOr<std::vector<uint8_t>> ParseHexString(const string& hex_string);

// Converts the given block of binary data to a human-readable string format.
// This function produces a sequence of two-letter hexadecimal codes separated
// by spaces.
//
// Example output format: 00 AB 01 BC.
template <typename Range>
string ToHumanReadableHexString(const Range& binary_data) {
  string buffer;
  for (const uint8_t encoding_byte : binary_data) {
    if (!buffer.empty()) {
      buffer.push_back(' ');
    }
    StringAppendF(&buffer, "%02X", static_cast<uint32_t>(encoding_byte));
  }
  return buffer;
}

// Converts the given block of binary data to a format that can be pased to C++
// source code as an array of uint8_t values.
//
// Example output format: 0x00, 0xAB, 0x01, 0xBC.
template <typename Range>
string ToPastableHexString(const Range& binary_data) {
  string buffer;
  for (const uint8_t encoding_byte : binary_data) {
    if (!buffer.empty()) {
      buffer.append(", ");
    }
    StringAppendF(&buffer, "0x%02X", static_cast<uint32_t>(encoding_byte));
  }
  return buffer;
}

}  // namespace cpu_instructions

#endif  // CPU_INSTRUCTIONS_UTIL_STRINGS_H_
