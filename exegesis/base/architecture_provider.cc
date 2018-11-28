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

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace {

using ::exegesis::util::InternalError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::NotFoundError;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Returns the registered providers.
absl::flat_hash_map<std::string,
                    std::unique_ptr<const ArchitectureProtoProvider>>*
GetProviders() {
  static auto* const registry = new absl::flat_hash_map<
      std::string, std::unique_ptr<const ArchitectureProtoProvider>>();
  return registry;
}

// Architecture provider for proto files.
template <Status (*Read)(const std::string&,
                         google::protobuf::Message* message)>
StatusOr<std::shared_ptr<const ArchitectureProto>> GetArchitectureProtoFromFile(
    const std::string& id) {
  auto result = std::make_shared<ArchitectureProto>();
  const Status read_status = Read(id, result.get());
  if (!read_status.ok()) return read_status;
  return std::shared_ptr<const ArchitectureProto>(result);
}

}  // namespace

StatusOr<std::shared_ptr<const ArchitectureProto>> GetArchitectureProto(
    const std::string& uri) {
  const auto sep = uri.find(':');
  // If sep is npos, source is the whole thing, and id is empty.
  const std::string source = uri.substr(0, sep);
  // Call GetProtoOrDie() even if the id is empty to give the provider a chance
  // to explain the issue.
  const std::string id = sep == std::string::npos ? "" : uri.substr(sep + 1);
  if (source == kPbTxtSource) {
    return GetArchitectureProtoFromFile<ReadTextProto>(id);
  } else if (source == kPbSource) {
    return GetArchitectureProtoFromFile<ReadBinaryProto>(id);
  } else if (source == kRegisteredSource) {
    const auto* provider = gtl::FindOrNull(*GetProviders(), id);
    if (provider == nullptr) {
      return NotFoundError(
          absl::StrCat("No ArchitectureProtoProvider registered for id '", id,
                       "'. Known ids are:\n",
                       absl::StrJoin(GetRegisteredArchitectureIds(), "\n")));
    }
    const auto proto_or_status = (*provider)->GetProto();
    if (proto_or_status.ok() && proto_or_status.ValueOrDie() == nullptr) {
      return InternalError("broken contract: GetProtoOrDie() != nullptr");
    }
    return proto_or_status;
  }
  return InvalidArgumentError(
      absl::StrCat("Unknown source '", source,
                   "'. If you meant to read from a text file, use ",
                   kPbTxtSource, ":/path/to/file"));
}

std::shared_ptr<const ArchitectureProto> GetArchitectureProtoOrDie(
    const std::string& uri) {
  const StatusOr<std::shared_ptr<const ArchitectureProto>>
      architecture_or_status = GetArchitectureProto(uri);
  CHECK_OK(architecture_or_status.status());
  return architecture_or_status.ValueOrDie();
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
