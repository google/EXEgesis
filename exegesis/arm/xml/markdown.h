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

#ifndef EXEGESIS_ARM_XML_MARKDOWN_H_
#define EXEGESIS_ARM_XML_MARKDOWN_H_

#include <string>

#include "tinyxml2.h"

namespace exegesis {
namespace arm {
namespace xml {

// Exports the HTML contained in the given XML element as Markdown, or returns
// an empty string if element is nullptr. This is a basic utility only dedicated
// to transforming <authored> description nodes present in ARM's XML instruction
// database, not any generic HTML content.
std::string ExportToMarkdown(const ::tinyxml2::XMLElement* element);

}  // namespace xml
}  // namespace arm
}  // namespace exegesis

#endif  // EXEGESIS_ARM_XML_MARKDOWN_H_
