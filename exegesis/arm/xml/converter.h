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

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_ARM_XML_CONVERTER_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_ARM_XML_CONVERTER_H_

#include "exegesis/arm/xml/parser.pb.h"
#include "exegesis/proto/instruction_encoding.pb.h"
#include "exegesis/proto/instructions.pb.h"

namespace exegesis {
namespace arm {
namespace xml {

ArchitectureProto ConvertToArchitectureProto(const XmlDatabase& xml_database);

}  // namespace xml
}  // namespace arm
}  // namespace exegesis

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_ARM_XML_CONVERTER_H_
