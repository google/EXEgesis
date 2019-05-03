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

#include "util/task/status.h"

namespace exegesis {

using ::exegesis::util::Status;

// Updates the value of 'status': if 'status' was not OK, its old value is kept.
// Otherwise, it is replaced with the value of 'new_status'.
void UpdateStatus(Status* status, const Status& new_status);

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_STATUS_UTIL_H_
