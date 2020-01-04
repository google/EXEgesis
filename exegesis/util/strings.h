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

#ifndef EXEGESIS_UTIL_STRINGS_H_
#define EXEGESIS_UTIL_STRINGS_H_

#include <cstdint>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::StatusOr;

// Parses the given hexadecimal string in several possible formats:
// * each byte is encoded as one or two hexadecimal digits,
// * each byte can have an optional 0x prefix,
// * both uppercase and lowercase letters are accepted,
// * the bytes are separated either by spaces or by commas.
//
// Example input formats:
// * 0x0,0x1,0x2,0x3
// * 00 AB 01 BC
StatusOr<std::vector<uint8_t>> ParseHexString(absl::string_view hex_string);

// Converts the given block of binary data to a human-readable string format.
// This function produces a sequence of two-letter hexadecimal codes separated
// by spaces.
//
// Example output format: 00 AB 01 BC.
template <typename Range>
std::string ToHumanReadableHexString(const Range& binary_data) {
  std::string buffer;
  for (const uint8_t encoding_byte : binary_data) {
    if (!buffer.empty()) {
      buffer.push_back(' ');
    }
    absl::StrAppendFormat(&buffer, "%02X", encoding_byte);
  }
  return buffer;
}

// Converts the given block of binary data to a format that can be pased to C++
// source code as an array of uint8_t values.
//
// Example output format: 0x00, 0xAB, 0x01, 0xBC.
template <typename Range>
std::string ToPastableHexString(const Range& binary_data) {
  std::string buffer;
  for (const uint8_t encoding_byte : binary_data) {
    if (!buffer.empty()) {
      buffer.append(", ");
    }
    absl::StrAppendFormat(&buffer, "0x%02X", encoding_byte);
  }
  return buffer;
}

// Converts a string-like data type to absl::string_view.
template <typename StringType>
absl::string_view ToStringView(const StringType& text) {
  return absl::string_view(text.data(), text.size());
}

// Converts a string-like data type to ::google::protobuf::StringPiece
template <typename StringType>
::google::protobuf::StringPiece ToStringPiece(const StringType& text) {
  return {text.data(), text.size()};
}

// Removes all chars in `chars` from `text`.
void RemoveAllChars(std::string* text, absl::string_view chars);

// Removes all space and line-feed characters from `text`.
void RemoveSpaceAndLF(std::string* text);

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_STRINGS_H_
