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

#ifndef UTIL_GTL_MAP_UTIL_H_
#define UTIL_GTL_MAP_UTIL_H_

#include "glog/logging.h"
// Required for GOOGLE_CHECK.
#include "src/google/protobuf/stubs/logging.h"
#include "src/google/protobuf/stubs/map_util.h"

namespace exegesis {
namespace gtl {

using ::google::protobuf::ContainsKey;
using ::google::protobuf::FindCopy;
using ::google::protobuf::FindOrDieNoPrint;
using ::google::protobuf::FindOrNull;
using ::google::protobuf::FindPtrOrNull;
using ::google::protobuf::FindWithDefault;
using ::google::protobuf::InsertIfNotPresent;
using ::google::protobuf::InsertKeysFromMap;
using ::google::protobuf::InsertOrDie;
using ::google::protobuf::InsertOrDieNoPrint;

template <typename Collection,
          typename Key = typename Collection::value_type::first_type,
          typename Value = typename Collection::value_type::second_type>
const Value& FindOrDie(const Collection& collection, const Key& key) {
  const auto it = collection.find(key);
  CHECK(it != collection.end()) << "Map key not found: " << key;
  return it->second;
}

}  // namespace gtl
}  // namespace exegesis

#endif  // UTIL_GTL_MAP_UTIL_H_
