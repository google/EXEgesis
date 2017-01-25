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

#include "cpu_instructions/x86/pdf/proto_util.h"

#include "glog/logging.h"
#include "src/google/protobuf/io/zero_copy_stream_impl.h"
#include "src/google/protobuf/text_format.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

void ReadTextProtoOrDie(const string& filename,
                        google::protobuf::Message* message) {
  CHECK(!filename.empty());
  FILE* const input_file = fopen(filename.c_str(), "rb");
  CHECK(input_file) << "Could not open '" << filename << "'";
  {
    google::protobuf::io::FileInputStream input_stream(fileno(input_file));
    CHECK(google::protobuf::TextFormat::Parse(&input_stream, message))
        << "Could not parse text format protobuf from file '" << filename
        << "'";
  }
  fclose(input_file);
}

void ParseProtoFromStringOrDie(const string& text,
                               google::protobuf::Message* message) {
  CHECK(google::protobuf::TextFormat::ParseFromString(text, message));
}

void WriteTextProtoOrDie(const string& filename,
                         const google::protobuf::Message& message) {
  CHECK(!filename.empty());
  FILE* const output_file = fopen(filename.c_str(), "wb");
  CHECK(output_file) << "Could not open '" << filename << "'";
  {
    google::protobuf::io::FileOutputStream output_stream(fileno(output_file));
    CHECK(google::protobuf::TextFormat::Print(message, &output_stream));
  }
  fclose(output_file);
}

void WriteBinaryProtoOrDie(const string& filename,
                           const google::protobuf::Message& message) {
  CHECK(!filename.empty());
  FILE* const output_file = fopen(filename.c_str(), "wb");
  CHECK(output_file) << "Could not open '" << filename << "'";
  CHECK(message.SerializeToFileDescriptor(fileno(output_file)));
  fclose(output_file);
}

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
