#ifndef THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_STRINGS_H_
#define THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_STRINGS_H_

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

#endif  // THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_STRINGS_H_
