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

#include "exegesis/util/file_util.h"

#include <cstdio>
#include <memory>

#include "glog/logging.h"

namespace exegesis {
namespace {

// Use a 1 MB buffer for the operations. This should be enough for most files
// that we deal with.
constexpr size_t kBufferSize = 1 << 20;

// Returns true if 'source_or_target' should be treated as a file name; returns
// false if STDIN/STDOUT should be used instead.
bool IsFileName(const std::string& source_or_target) {
  return source_or_target != "-";
}

}  // namespace

std::string ReadTextFromFileOrStdInOrDie(const std::string& source) {
  CHECK(!source.empty());
  const bool is_file = IsFileName(source);
  FILE* const input = is_file ? fopen(source.c_str(), "rb") : stdin;
  CHECK(input != nullptr) << "Could not open '" << source << "'";
  std::unique_ptr<char[]> buffer(new char[kBufferSize]);
  std::string out;
  size_t read_items = 0;
  do {
    read_items = fread(buffer.get(), sizeof(char), kBufferSize, input);
    out.append(buffer.get(), read_items);
  } while (read_items == kBufferSize);
  CHECK(feof(input));
  if (is_file) fclose(input);
  return out;
}

void WriteTextToFileOrStdOutOrDie(const std::string& target,
                                  const std::string& data) {
  CHECK(!target.empty());
  const bool is_file = IsFileName(target);
  FILE* const output = is_file ? fopen(target.c_str(), "wb") : stdout;
  CHECK(output != nullptr) << "Could not open '" << target << "'";
  const size_t written_items =
      fwrite(data.data(), sizeof(char), data.size(), output);
  CHECK_EQ(written_items, data.size());
  fflush(output);
  if (is_file) fclose(output);
}

}  // namespace exegesis
