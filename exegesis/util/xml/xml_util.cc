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

#include "exegesis/util/xml/xml_util.h"

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "exegesis/util/status_util.h"
#include "glog/logging.h"
#include "tinyxml2.h"

namespace exegesis {
namespace xml {

namespace {

using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;
using ::tinyxml2::XMLError;
using ::tinyxml2::XMLNode;
using ::tinyxml2::XMLPrinter;

}  // namespace

absl::Status GetStatus(const XMLError& status) {
  if (status == XMLError::XML_SUCCESS) return absl::OkStatus();
  // Use a dummy document to print a user-friendly error string.
  XMLDocument dummy;
  return absl::InternalError(
      absl::StrCat("XML Error ", status, ": ", dummy.ErrorIDToName(status)));
}

std::string DebugString(const XMLNode* node) {
  CHECK(node != nullptr);
  XMLPrinter printer;
  node->Accept(&printer);
  return printer.CStr();
}

absl::StatusOr<XMLElement*> FindChild(XMLNode* node, const char* name) {
  CHECK(node != nullptr);
  XMLElement* element = node->FirstChildElement(name);
  if (element != nullptr) return element;
  return absl::NotFoundError(absl::StrCat(
      "Element <", name ? name : "", "> not found in:\n", DebugString(node)));
}

std::vector<XMLElement*> FindChildren(XMLNode* node, const char* name) {
  CHECK(node != nullptr);
  std::vector<XMLElement*> children;
  for (XMLElement* element = node->FirstChildElement(name); element != nullptr;
       element = element->NextSiblingElement(name)) {
    children.push_back(element);
  }
  return children;
}

std::string ReadAttribute(const XMLElement* element, const char* name) {
  CHECK(element != nullptr);
  CHECK(name != nullptr);
  const char* value = element->Attribute(name);
  return value ? value : "";
}

absl::StatusOr<int> ReadIntAttribute(const XMLElement* element,
                                     const char* name) {
  CHECK(element != nullptr);
  CHECK(name != nullptr);
  int value = -1;
  RETURN_IF_ERROR(GetStatus(element->QueryIntAttribute(name, &value)));
  return value;
}

int ReadIntAttributeOrDefault(const XMLElement* element, const char* name,
                              int default_value) {
  CHECK(element != nullptr);
  CHECK(name != nullptr);
  const auto value = ReadIntAttribute(element, name);
  return value.ok() ? value.value() : default_value;
}

std::string ReadSimpleText(const XMLElement* element) {
  CHECK(element != nullptr);
  const char* text = element->GetText();
  return text ? text : "";
}

std::string ReadHtmlText(const XMLElement* element) {
  CHECK(element != nullptr);
  XMLPrinter printer(nullptr, /* compact = */ true, /* depth = */ 0);
  element->Accept(&printer);
  CHECK(printer.CStr() != nullptr);
  return printer.CStr();
}

}  // namespace xml
}  // namespace exegesis
