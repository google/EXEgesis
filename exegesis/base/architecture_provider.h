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

// A provider system for ArchitectureProtos. This is here so that code that
// is agnostic to any specific architecture can still create ArchitectureProtos
// for specific architectures.

#ifndef EXEGESIS_BASE_ARCHITECTURE_PROVIDER_H_
#define EXEGESIS_BASE_ARCHITECTURE_PROVIDER_H_

#include <memory>
#include <string>

#include "exegesis/proto/instructions.pb.h"
#include "util/gtl/ptr_util.h"

namespace exegesis {

constexpr const char kPbTxtSource[] = "pbtxt";
constexpr const char kPbSource[] = "pb";
constexpr const char kRegisteredSource[] = "registered";

// Returns the architecture proto for then given architecture uri.
// The uri has the following format:
//  <source>:<id>
// Where <source> is the source name, and <id> is a source-specific opaque
// string. <source> can be:
//   - 'pbtxt': <id> is a file name where the architecture is stored in text
//     format. Example: 'pbtxt:/path/to/file.pbtxt'
//   - 'pb': <id> is a file name where the architecture is stored in binary
//     format. Example: 'pb:/path/to/binary_proto.pb'
//   - 'registered': <id> corresponds the name of a provider that was registered
//     using REGISTER_ARCHITECTURE_PROTO_PROVIDER.
// Dies with a useful error message if the provider is not found or if it
// returns an error. On success the result is guaranteed to be non-null.
std::shared_ptr<const ArchitectureProto> GetArchitectureProtoOrDie(
    const std::string& uri);

// Returns the list of registered architectures.
std::vector<std::string> GetRegisteredArchitectureIds();

// See top comment.
class ArchitectureProtoProvider {
 public:
  virtual ~ArchitectureProtoProvider();

  // This is a shared_ptr because some providers are going to hold singletons
  // while some others will reliquish ownership (and the proto itself is huge).
  // Dies on error and never returns nullptr.
  virtual std::shared_ptr<const ArchitectureProto> GetProtoOrDie() const = 0;

 protected:
  ArchitectureProtoProvider() = default;
};

// Registers an ArchitectureProtoProvider for the called 'provider_name'.
#define REGISTER_ARCHITECTURE_PROTO_PROVIDER(provider_name, Type) \
  ::exegesis::internal::RegisterArchitectureProtoProvider         \
      register_architectures_provider##Type(provider_name,        \
                                            gtl::MakeUnique<Type>());

namespace internal {

// A helper class used for the implementation of the registerer.
class RegisterArchitectureProtoProvider {
 public:
  RegisterArchitectureProtoProvider(
      const std::string& provider_name,
      std::unique_ptr<ArchitectureProtoProvider> provider);
};

}  // namespace internal

}  // namespace exegesis

#endif  // EXEGESIS_BASE_ARCHITECTURE_PROVIDER_H_
