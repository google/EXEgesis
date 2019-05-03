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

#include "exegesis/testing/test_util.h"

#include "gtest/gtest.h"

namespace exegesis {
namespace testing {
namespace internal {

using ::google::protobuf::Descriptor;
using ::google::protobuf::DescriptorPool;
using ::google::protobuf::FieldDescriptor;

void AddIgnoredFieldsToDifferencer(
    const ::google::protobuf::Descriptor* descriptor,
    const std::vector<std::string>& ignored_field_names,
    ::google::protobuf::util::MessageDifferencer* differencer) {
  CHECK(descriptor != nullptr);
  CHECK(differencer != nullptr);
  const DescriptorPool* const pool = descriptor->file()->pool();
  for (const std::string& field_name : ignored_field_names) {
    const FieldDescriptor* const field = pool->FindFieldByName(field_name);
    if (field == nullptr) {
      ADD_FAILURE() << "Field \"" << field_name << "\" was not found.";
      continue;
    }
    differencer->IgnoreField(field);
  }
}

}  // namespace internal
}  // namespace testing
}  // namespace exegesis
