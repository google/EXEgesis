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

#ifndef NET_PROTO2_UTIL_PUBLIC_REPEATED_FIELD_UTIL_H_
#define NET_PROTO2_UTIL_PUBLIC_REPEATED_FIELD_UTIL_H_

#include <algorithm>
#include <cassert>
#include <utility>

#include "src/google/protobuf/repeated_field.h"

namespace exegesis {

template <typename T>
void Truncate(::google::protobuf::RepeatedPtrField<T>* array, int new_size) {
  const int size = array->size();
  assert(size >= new_size);
  array->DeleteSubrange(new_size, size - new_size);
}

template <typename T, typename Pred>
int RemoveIf(::google::protobuf::RepeatedPtrField<T>* array, const Pred& pr) {
  T** const begin = array->mutable_data();
  T** const end = begin + array->size();
  T** write = begin;
  while (write < end && !pr(*write)) ++write;
  if (write == end) return 0;
  // 'write' is positioned at first element to be removed.
  for (T** scan = write + 1; scan < end; ++scan) {
    if (!pr(*scan)) std::swap(*scan, *write++);
  }
  Truncate(array, write - begin);
  return end - write;
}

template <typename T, typename LessThan>
inline void Sort(::google::protobuf::RepeatedPtrField<T>* array,
                 const LessThan& lt) {
  std::sort(array->pointer_begin(), array->pointer_end(), lt);
}

template <typename T, typename LessThan>
inline void Sort(::google::protobuf::RepeatedField<T>* array,
                 const LessThan& lt) {
  std::sort(array->begin(), array->end(), lt);
}

template <typename T>
inline void Sort(::google::protobuf::RepeatedField<T>* array) {
  std::sort(array->begin(), array->end());
}

}  // namespace exegesis

#endif  // NET_PROTO2_UTIL_PUBLIC_REPEATED_FIELD_UTIL_H_
