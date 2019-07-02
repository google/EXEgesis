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

#include "exegesis/base/transform_factory.h"

#include <string>

#include "absl/flags/flag.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

ABSL_FLAG(
    std::string, exegesis_transforms, "",
    "The list of transformations applied to the instruction "
    "database. This can be a (possibly empty) list of names or "
    "'default' to apply the default list of transforms for the architecture.");

namespace exegesis {

std::vector<InstructionSetTransform> GetTransformsFromCommandLineFlags() {
  static constexpr const char kDefaultSet[] = "default";
  const auto& transforms_by_name = GetTransformsByName();
  std::vector<InstructionSetTransform> transforms;
  const std::vector<std::string> transform_names =
      absl::StrSplit(absl::GetFlag(FLAGS_exegesis_transforms), ",",  // NOLINT
                     absl::SkipEmpty());
  for (const std::string& transform_name : transform_names) {
    if (transform_name == kDefaultSet) {
      const auto default_transforms = GetDefaultTransformPipeline();
      transforms.insert(transforms.end(), default_transforms.begin(),
                        default_transforms.end());
    } else {
      auto* transform = gtl::FindOrNull(transforms_by_name, transform_name);
      CHECK(transform != nullptr)
          << "Transform was not found: " << transform_name;
      transforms.push_back(*transform);
    }
  }
  return transforms;
}

}  // namespace exegesis
