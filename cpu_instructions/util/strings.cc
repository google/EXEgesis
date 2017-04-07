#include "cpu_instructions/util/strings.h"

#include "base/stringprintf.h"
#include "re2/re2.h"
#include "strings/str_cat.h"
#include "util/task/canonical_errors.h"

namespace cpu_instructions {

using ::cpu_instructions::util::StatusOr;

StatusOr<std::vector<uint8_t>> ParseHexString(const string& hex_string) {
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

}  // namespace cpu_instructions
