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

#ifndef EXEGESIS_UTIL_STATUS_UTIL_H_
#define EXEGESIS_UTIL_STATUS_UTIL_H_

#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace exegesis {

#ifndef CHECK_OK
#define CHECK_OK(status_expr)                  \
  do {                                         \
    const absl::Status status = (status_expr); \
    CHECK(status.ok()) << status;              \
  } while (false)
#endif  // CHECK_OK

#ifndef RETURN_IF_ERROR
#define RETURN_IF_ERROR(status_expr)   \
  do {                                 \
    const auto status = (status_expr); \
    if (!status.ok()) return status;   \
  } while (false)
#endif  // RETURN_IF_ERROR

inline absl::Status AnnotateStatus(const absl::Status& s,
                                   absl::string_view msg) {
  if (s.ok() || msg.empty()) return s;
  const std::string annotated = s.message().empty()
                                    ? std::string(msg)
                                    : absl::StrCat(s.message(), "; ", msg);
  return absl::Status(s.code(), annotated);
}

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_STATUS_UTIL_H_
