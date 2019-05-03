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

// Utilities to manipulate XML documents with tinyxml2.

#ifndef EXEGESIS_UTIL_XML_XML_UTIL_H_
#define EXEGESIS_UTIL_XML_XML_UTIL_H_

#include <string>
#include <vector>

#include "tinyxml2.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace xml {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Transforms an XMLError code into a proper Status.
Status GetStatus(const ::tinyxml2::XMLError& status);

// Returns the XML string representation of the given node.
std::string DebugString(const ::tinyxml2::XMLNode* node);

// Returns the first direct child element of node with the specified name.
// If name is nullptr, finds the first child element regardless of its name.
StatusOr<::tinyxml2::XMLElement*> FindChild(::tinyxml2::XMLNode* node,
                                            const char* name);

// Returns all direct children elements of node with the specified name.
// If name is nullptr, finds all children elements regardless of their names.
std::vector<::tinyxml2::XMLElement*> FindChildren(::tinyxml2::XMLNode* node,
                                                  const char* name);

// Reads the specified attribute from the given element as a string.
// Returns an empty string if no such attribute is found.
std::string ReadAttribute(const ::tinyxml2::XMLElement* element,
                          const char* name);

// Reads the specified attribute from the given element as an integer.
// Returns an error if no such attribute is found or if it can't be parsed.
StatusOr<int> ReadIntAttribute(const ::tinyxml2::XMLElement* element,
                               const char* name);

// Reads the specified attribute from the given element as an integer.
// Returns default_value if no such attribute is found or if it can't be parsed.
int ReadIntAttributeOrDefault(const ::tinyxml2::XMLElement* element,
                              const char* name, int default_value);

// Reads the text lying directly inside the given element, skipping nested tags.
std::string ReadSimpleText(const ::tinyxml2::XMLElement* element);

// Reads the element as a full HTML text, also considering nested tags.
std::string ReadHtmlText(const ::tinyxml2::XMLElement* element);

}  // namespace xml
}  // namespace exegesis

#endif  // EXEGESIS_UTIL_XML_XML_UTIL_H_
