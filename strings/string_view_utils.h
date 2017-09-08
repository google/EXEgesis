// Copyright 2016 Google Inc.
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

#ifndef STRINGS_STRING_VIEW_UTILS_H_
#define STRINGS_STRING_VIEW_UTILS_H_

#include "src/google/protobuf/stubs/strutil.h"

namespace exegesis {
namespace strings {

inline bool StartsWith(::google::protobuf::StringPiece str,
                       ::google::protobuf::StringPiece prefix) {
  return ::google::protobuf::HasPrefixString(str, prefix);
}

inline bool EndsWith(::google::protobuf::StringPiece str,
                     ::google::protobuf::StringPiece suffix) {
  return ::google::protobuf::HasSuffixString(str, suffix);
}

inline bool ConsumePrefix(::google::protobuf::StringPiece* str,
                          ::google::protobuf::StringPiece expected) {
  if (!StartsWith(*str, expected)) return false;
  str->remove_prefix(expected.size());
  return true;
}

inline ssize_t RemoveLeadingWhitespace(::google::protobuf::StringPiece* text) {
  size_t count = 0;
  const char* ptr = text->data();
  while (count < text->size() && ::google::protobuf::ascii_isspace(*ptr)) {
    count++;
    ptr++;
  }
  text->remove_prefix(count);
  return count;
}

inline ssize_t RemoveTrailingWhitespace(::google::protobuf::StringPiece* text) {
  size_t count = 0;
  const char* ptr = text->data() + text->size() - 1;
  while (count < text->size() && ::google::protobuf::ascii_isspace(*ptr)) {
    ++count;
    --ptr;
  }
  text->remove_suffix(count);
  return count;
}

inline ssize_t RemoveWhitespaceContext(::google::protobuf::StringPiece* text) {
  return RemoveLeadingWhitespace(text) + RemoveTrailingWhitespace(text);
}

}  // namespace strings
}  // namespace exegesis

#endif  // STRINGS_STRING_VIEW_UTILS_H_
