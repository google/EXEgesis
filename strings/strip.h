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

#ifndef STRINGS_STRIP_H_
#define STRINGS_STRIP_H_

#include "strings/string.h"

#include "src/google/protobuf/stubs/strutil.h"

namespace exegesis {

using ::google::protobuf::StripWhitespace;

inline ptrdiff_t strrmm(string* str, const string& chars) {
  size_t str_len = str->length();
  size_t in_index = str->find_first_of(chars);
  if (in_index == string::npos) return str_len;

  size_t out_index = in_index++;

  while (in_index < str_len) {
    char c = (*str)[in_index++];
    if (chars.find(c) == string::npos) (*str)[out_index++] = c;
  }

  str->resize(out_index);
  return out_index;
}

}  // namespace exegesis

#endif  // STRINGS_STRIP_H_
