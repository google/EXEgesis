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

#ifndef EXEGESIS_ARM_XML_PARSER_H_
#define EXEGESIS_ARM_XML_PARSER_H_

#include <string>

#include "exegesis/arm/xml/parser.pb.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace arm {
namespace xml {

using ::exegesis::util::StatusOr;

// Parses the specified XML database index file.
StatusOr<XmlIndex> ParseXmlIndex(const std::string& filename);

// Parses the specified XML instruction file.
StatusOr<XmlInstruction> ParseXmlInstruction(const std::string& filename);

// Parses the ARM XML instruction database, reading files from the given path.
StatusOr<XmlDatabase> ParseXmlDatabase(const std::string& path);

// Same as ParseXmlDatabase, but dies on errors.
XmlDatabase ParseXmlDatabaseOrDie(const std::string& path);

}  // namespace xml
}  // namespace arm
}  // namespace exegesis

#endif  // EXEGESIS_ARM_XML_PARSER_H_
