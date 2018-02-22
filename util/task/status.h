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

#ifndef UTIL_TASK_STATUS_H_
#define UTIL_TASK_STATUS_H_

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "src/google/protobuf/stubs/status.h"

namespace exegesis {
namespace util {
namespace error {

using ::google::protobuf::util::error::Code;
using ::google::protobuf::util::error::FAILED_PRECONDITION;
using ::google::protobuf::util::error::INTERNAL;
using ::google::protobuf::util::error::INVALID_ARGUMENT;
using ::google::protobuf::util::error::NOT_FOUND;
using ::google::protobuf::util::error::OK;
using ::google::protobuf::util::error::UNIMPLEMENTED;
using ::google::protobuf::util::error::UNKNOWN;

}  // namespace error

using ::google::protobuf::util::Status;
using ::google::protobuf::util::operator<<;

inline Status Annotate(const Status& s, absl::string_view msg) {
  if (s.ok() || msg.empty()) return s;
  const std::string annotated =
      s.error_message().empty()
          ? std::string(msg)
          : absl::StrCat(s.error_message().as_string(), "; ", msg);
  return Status(static_cast<error::Code>(s.error_code()), annotated);
}

inline Status OkStatus() { return Status(); }

#ifndef CHECK_OK
#define CHECK_OK(status_expr)                              \
  do {                                                     \
    const ::exegesis::util::Status status = (status_expr); \
    CHECK(status.ok()) << status;                          \
  } while (false)
#endif  // CHECK_OK

}  // namespace util
}  // namespace exegesis

#endif  // UTIL_TASK_STATUS_H_
