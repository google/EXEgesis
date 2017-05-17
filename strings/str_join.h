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

#ifndef STRINGS_STR_JOIN_H_
#define STRINGS_STR_JOIN_H_

#include <vector>
#include "strings/string.h"

#include "src/google/protobuf/stubs/strutil.h"

namespace exegesis {
namespace strings {

using ::google::protobuf::Join;

inline std::string Join(const std::vector<std::string>& components,
                        const char* delim) {
  return ::google::protobuf::JoinStrings(components, delim);
}

template <typename Range, typename Formatter>
std::string Join(const Range& components, const char* delim,
                 const Formatter& formatter) {
  std::vector<std::string> str_components;
  for (const auto& component : components) {
    std::string str;
    formatter(&str, component);
    str_components.push_back(std::move(str));
  }
  return ::google::protobuf::JoinStrings(str_components, delim);
}

}  // namespace strings
}  // namespace exegesis

#endif  // STRINGS_STR_JOIN_H_
