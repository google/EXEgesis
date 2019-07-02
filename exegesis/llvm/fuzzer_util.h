// Copyright 2019 Google Inc.
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

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_LLVM_FUZZER_UTIL_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_LLVM_FUZZER_UTIL_H_

#include <string>

#include "absl/strings/string_view.h"

// Since AssembleDisassemble gets confused about $ before the constants, we need
// to escape them, by prepending another $.
std::string EscapeDollars(absl::string_view str);

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_LLVM_FUZZER_UTIL_H_
