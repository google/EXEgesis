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

#include "strings/string.h"

#include "strings/str_cat.h"
#include "strings/string_view.h"
#include "strings/string_view_utils.h"

namespace exegesis {
namespace file {

string JoinPath(StringPiece a, StringPiece b) {
  if (a.empty()) return string(b);
  if (b.empty()) return string(a);
  if (strings::EndsWith(a, "/")) {
    if (strings::StartsWith(b, "/")) return StrCat(a, b.substr(1));
  } else {
    if (!strings::StartsWith(b, "/")) return StrCat(a, "/", b);
  }
  return StrCat(a, b);
}

}  // namespace file
}  // namespace exegesis
