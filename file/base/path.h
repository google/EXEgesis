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

#ifndef FILE_BASE_PATH_H_
#define FILE_BASE_PATH_H_

#include <string>

#include "absl/strings/string_view.h"

namespace exegesis {
namespace file {

// Joins multiple paths together. All paths will be treated as relative paths,
// regardless of whether or not they start with a leading '/'. That is, all
// paths will be concatenated together, with the appropriate path separator
// inserted in between.
std::string JoinPath(absl::string_view a, absl::string_view b);

}  // namespace file
}  // namespace exegesis

#endif  // FILE_BASE_PATH_H_
