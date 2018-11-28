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

#include "exegesis/util/proto_util.h"

#include "absl/strings/str_cat.h"
#include "glog/logging.h"
#include "src/google/protobuf/io/zero_copy_stream_impl.h"
#include "src/google/protobuf/text_format.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

struct FileCloser {
  void operator()(FILE* file) {
    if (file != nullptr) fclose(file);
  }
};

Status ReadTextProto(const std::string& filename,
                     google::protobuf::Message* message) {
  if (filename.empty()) {
    return InvalidArgumentError("filename must not be empty");
  }
  std::unique_ptr<FILE, FileCloser> input_file(fopen(filename.c_str(), "rb"));
  if (input_file == nullptr) {
    // TODO(ondrasej): Implement a proper translation from Unix errno codes to
    // ::util::Status codes.
    return FailedPreconditionError(
        absl::StrCat("Could not open '", filename, "'"));
  }
  google::protobuf::io::FileInputStream input_stream(fileno(input_file.get()));
  if (!google::protobuf::TextFormat::Parse(&input_stream, message)) {
    return FailedPreconditionError(absl::StrCat(
        "Could not parse text format protobuf from file '", filename, "'"));
  }
  return OkStatus();
}

Status ReadBinaryProto(const std::string& filename,
                       google::protobuf::Message* message) {
  if (filename.empty()) {
    return InvalidArgumentError("filename must not be empty");
  }
  std::unique_ptr<FILE, FileCloser> input_file(fopen(filename.c_str(), "rb"));
  if (input_file == nullptr) {
    // TODO(ondrasej): Implement a proper translation from Unix errno codes to
    // ::util::Status codes.
    return FailedPreconditionError(
        absl::StrCat("Could not open '", filename, "'"));
  }
  if (!message->ParseFromFileDescriptor(fileno(input_file.get()))) {
    return FailedPreconditionError(absl::StrCat(
        "Could not parse binary format protobuf from file '", filename, "'"));
  }
  return OkStatus();
}

void ParseProtoFromStringOrDie(absl::string_view text,
                               google::protobuf::Message* message) {
  google::protobuf::io::ArrayInputStream input_stream(text.data(), text.size());
  CHECK(google::protobuf::TextFormat::Parse(&input_stream, message));
}

void WriteTextProtoOrDie(const std::string& filename,
                         const google::protobuf::Message& message,
                         const google::protobuf::TextFormat::Printer& printer) {
  CHECK(!filename.empty());
  FILE* const output_file = fopen(filename.c_str(), "wb");
  CHECK(output_file) << "Could not open '" << filename << "'";
  {
    google::protobuf::io::FileOutputStream output_stream(fileno(output_file));
    CHECK(printer.Print(message, &output_stream));
  }
  fclose(output_file);
}

void WriteBinaryProtoOrDie(const std::string& filename,
                           const google::protobuf::Message& message) {
  CHECK(!filename.empty());
  FILE* const output_file = fopen(filename.c_str(), "wb");
  CHECK(output_file) << "Could not open '" << filename << "'";
  CHECK(message.SerializeToFileDescriptor(fileno(output_file)));
  fclose(output_file);
}

}  // namespace exegesis
