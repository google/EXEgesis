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

#include "file/base/path.h"

#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace exegesis {
namespace file {

std::string JoinPath(absl::string_view a, absl::string_view b) {
  if (a.empty()) return std::string(b);
  if (b.empty()) return std::string(a);
  if (absl::EndsWith(a, "/")) {
    if (absl::StartsWith(b, "/")) return absl::StrCat(a, b.substr(1));
  } else {
    if (!absl::StartsWith(b, "/")) return absl::StrCat(a, "/", b);
  }
  return absl::StrCat(a, b);
}

}  // namespace file
}  // namespace exegesis
