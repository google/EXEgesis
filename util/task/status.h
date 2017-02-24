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

#include "src/google/protobuf/stubs/status.h"

namespace cpu_instructions {
namespace util {
namespace error {

using ::google::protobuf::util::error::Code;
using ::google::protobuf::util::error::INTERNAL;
using ::google::protobuf::util::error::INVALID_ARGUMENT;
using ::google::protobuf::util::error::FAILED_PRECONDITION;

}  // namespace error

using ::google::protobuf::util::Status;
using ::google::protobuf::util::operator<<;

inline Status OkStatus() { return Status(); }

#ifndef CHECK_OK
#define CHECK_OK(status_expr)                                      \
  do {                                                             \
    const ::cpu_instructions::util::Status status = (status_expr); \
    CHECK(status.ok()) << status;                                  \
  } while (false)
#endif  // CHECK_OK

#ifndef ASSERT_OK
#define ASSERT_OK(status_expr)                                     \
  do {                                                             \
    const ::cpu_instructions::util::Status status = (status_expr); \
    ASSERT_TRUE(status.ok()) << status;                            \
  } while (false)
#endif  // ASSERT_OK
#ifndef EXPECT_OK
#define EXPECT_OK(status_expr)                                     \
  do {                                                             \
    const ::cpu_instructions::util::Status status = (status_expr); \
    EXPECT_TRUE(status.ok()) << status;                            \
  } while (false)
#endif  // EXPECT_OK

}  // namespace util
}  // namespace cpu_instructions

#endif  // UTIL_TASK_STATUS_H_
