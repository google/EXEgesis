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

#ifndef EXEGESIS_UTIL_FILE_UTIL_H_
#define EXEGESIS_UTIL_FILE_UTIL_H_

#include <string>

namespace exegesis {

// Reads all data from the given source, until reaching EOF. When 'source' is
// '-', the function reads the data fromm STDIN. Otherwise, assumes that
// 'source' is the name of a file and reads from that file.
std::string ReadTextFromFileOrStdInOrDie(const std::string& source);

// Writes 'text' to the given target. When 'target' is '-', the text is written
// to STDOUT. Otherwise, assumes that 'target' is the name of the file, and
// writes the data to that file, replacing the original contents.
void WriteTextToFileOrStdOutOrDie(const std::string& target,
                                  const std::string& data);

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_FILE_UTIL_H_
