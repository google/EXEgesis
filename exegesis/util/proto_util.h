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

#ifndef EXEGESIS_UTIL_PROTO_UTIL_H_
#define EXEGESIS_UTIL_PROTO_UTIL_H_

#include <string>

#include "absl/strings/string_view.h"
#include "src/google/protobuf/message.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Reads a proto in text format from a file.
Status ReadTextProto(const std::string& filename,
                     ::google::protobuf::Message* message);

// Typed versions of the above.
template <typename Proto>
StatusOr<Proto> ReadTextProto(const std::string& filename) {
  Proto proto;
  const Status read_status = ReadTextProto(filename, &proto);
  if (!read_status.ok()) return read_status;
  return proto;
}

template <typename Proto>
Proto ReadTextProtoOrDie(const std::string& filename) {
  return ReadTextProto<Proto>(filename).ValueOrDie();
}

// Reads a proto in binary format from a file.
Status ReadBinaryProto(const std::string& filename,
                       ::google::protobuf::Message* message);

// Typed versions of the above.
template <typename Proto>
StatusOr<Proto> ReadBinaryProto(const std::string& filename) {
  Proto proto;
  const Status read_status = ReadBinaryProto(filename, &proto);
  if (!read_status.ok()) return read_status;
  return proto;
}

template <typename Proto>
Proto ReadBinaryProtoOrDie(const std::string& filename) {
  return ReadBinaryProto<Proto>(filename).ValueOrDie();
}

// Reads a proto in text format from a string.
void ParseProtoFromStringOrDie(absl::string_view text,
                               ::google::protobuf::Message* message);

// Typed version of the above.
template <typename Proto>
Proto ParseProtoFromStringOrDie(absl::string_view text) {
  Proto proto;
  ParseProtoFromStringOrDie(text, &proto);
  return proto;
}

// Writes a proto in text format to a file.
void WriteTextProtoOrDie(const std::string& filename,
                         const google::protobuf::Message& message,
                         const google::protobuf::TextFormat::Printer& printer =
                             google::protobuf::TextFormat::Printer());

// Writes a proto in binary format to a file.
void WriteBinaryProtoOrDie(const std::string& filename,
                           const google::protobuf::Message& message);

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_PROTO_UTIL_H_
