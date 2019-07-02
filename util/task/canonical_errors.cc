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

#include "util/task/canonical_errors.h"

#include "util/task/status.h"

namespace exegesis {
namespace util {
namespace {

::google::protobuf::StringPiece ToStringPiece(absl::string_view text) {
  return ::google::protobuf::StringPiece(text.data(), text.size());
}

}  // namespace

Status FailedPreconditionError(absl::string_view error_message) {
  return Status(error::FAILED_PRECONDITION, ToStringPiece(error_message));
}

Status InternalError(absl::string_view error_message) {
  return Status(error::INTERNAL, ToStringPiece(error_message));
}

Status InvalidArgumentError(absl::string_view error_message) {
  return Status(error::INVALID_ARGUMENT, ToStringPiece(error_message));
}

Status NotFoundError(absl::string_view error_message) {
  return Status(error::NOT_FOUND, ToStringPiece(error_message));
}

Status UnimplementedError(absl::string_view error_message) {
  return Status(error::UNIMPLEMENTED, ToStringPiece(error_message));
}

Status UnknownError(absl::string_view error_message) {
  return Status(error::UNKNOWN, ToStringPiece(error_message));
}

bool IsNotFound(const Status& status) {
  return status.error_code() == error::NOT_FOUND;
}

bool IsInvalidArgument(const Status& status) {
  return status.error_code() == error::INVALID_ARGUMENT;
}

}  // namespace util
}  // namespace exegesis
