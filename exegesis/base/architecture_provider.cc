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

#include "exegesis/base/architecture_provider.h"

#include <tuple>
#include <unordered_map>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {

namespace {

// Returns the registered providers.
std::unordered_map<std::string,
                   std::unique_ptr<const ArchitectureProtoProvider>>*
GetProviders() {
  static auto* const registry = new std::unordered_map<
      std::string, std::unique_ptr<const ArchitectureProtoProvider>>();
  return registry;
}

// Architecture provider for proto files.
template <void (*Read)(const std::string&, google::protobuf::Message* message)>
std::shared_ptr<const ArchitectureProto> GetArchitectureProtoFromFileOrDie(
    const std::string& id) {
  auto result = std::make_shared<ArchitectureProto>();
  Read(id, result.get());
  return result;
}

}  // namespace

std::shared_ptr<const ArchitectureProto> GetArchitectureProtoOrDie(
    const std::string& uri) {
  const auto sep = uri.find(':');
  // If sep is npos, source is the whole thing, and id is empty.
  const std::string source = uri.substr(0, sep);
  // Call GetProtoOrDie() even if the id is empty to give the provider a chance
  // to explain the issue.
  const std::string id = sep == std::string::npos ? "" : uri.substr(sep + 1);
  if (source == kPbTxtSource) {
    return GetArchitectureProtoFromFileOrDie<ReadTextProtoOrDie>(id);
  } else if (source == kPbSource) {
    return GetArchitectureProtoFromFileOrDie<ReadBinaryProtoOrDie>(id);
  } else if (source == kRegisteredSource) {
    const auto* provider = FindOrNull(*GetProviders(), id);
    CHECK(provider) << "No ArchitectureProtoProvider registered for id '" << id
                    << "'. Known ids are:\n"
                    << absl::StrJoin(GetRegisteredArchitectureIds(), "\n");
    auto proto = (*provider)->GetProtoOrDie();
    CHECK(proto) << "broken contract: GetProtoOrDie() != nullptr";
    return proto;
  }
  LOG(FATAL) << "Unknown source '" << source
             << "'. If you meant to read from a text file, use " << kPbTxtSource
             << ":/path/to/file";
  return nullptr;
}

std::vector<std::string> GetRegisteredArchitectureIds() {
  std::vector<std::string> result;
  for (const auto& name_provider : *GetProviders()) {
    result.push_back(name_provider.first);
  }
  return result;
}

ArchitectureProtoProvider::~ArchitectureProtoProvider() {}

internal::RegisterArchitectureProtoProvider::RegisterArchitectureProtoProvider(
    const std::string& name,
    std::unique_ptr<ArchitectureProtoProvider> provider) {
  const bool inserted =
      GetProviders()->emplace(name, std::move(provider)).second;
  CHECK(inserted) << "Duplicate provider '" << name << "'";
}

}  // namespace exegesis
