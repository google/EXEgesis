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

#include "exegesis/arm/xml/markdown.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "absl/strings/str_cat.h"
#include "exegesis/util/xml/xml_util.h"
#include "re2/re2.h"
#include "tinyxml2.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace arm {
namespace xml {

namespace {

using ::exegesis::xml::ReadAttribute;
using ::tinyxml2::XMLAttribute;
using ::tinyxml2::XMLElement;
using ::tinyxml2::XMLText;
using ::tinyxml2::XMLVisitor;

enum class TagType {
  UNKNOWN,
  BLOCKQUOTE,
  CODE,
  IMAGE,
  LINK,
  LIST,
  LISTITEM,
  PARAGRAPH,
};

// Returns the canonical type of a given tag.
TagType GetType(const XMLElement& element) {
  static const auto* const kTagTypes =
      new std::unordered_map<std::string, TagType>({
          {"arm-defined-word", TagType::CODE},
          {"hexnumber", TagType::CODE},
          {"image", TagType::IMAGE},
          {"instruction", TagType::CODE},
          {"list", TagType::LIST},
          {"listitem", TagType::LISTITEM},
          {"note", TagType::BLOCKQUOTE},
          {"para", TagType::PARAGRAPH},
          {"syntax", TagType::CODE},
          {"value", TagType::CODE},
          {"xref", TagType::LINK},
      });
  return FindWithDefault(*kTagTypes, element.Name(), TagType::UNKNOWN);
}

// TinyXML Visitor to parse simple HTML as Markdown.
class TinyMarkdownParser : public XMLVisitor {
 public:
  bool VisitEnter(const XMLElement& element,
                  const XMLAttribute* first_attribute) override {
    switch (GetType(element)) {
      case TagType::BLOCKQUOTE:
        absl::StrAppend(&md_, "\n\n> ");
        break;
      case TagType::CODE:
        absl::StrAppend(&md_, "`");
        break;
      case TagType::IMAGE:
        absl::StrAppend(&md_, "![", ReadAttribute(&element, "label"), "]");
        absl::StrAppend(&md_, "(", ReadAttribute(&element, "file"), ")");
        break;
      case TagType::LINK:
        absl::StrAppend(&md_, "[");
        break;
      case TagType::LIST:
        absl::StrAppend(&md_, "\n");
        break;
      case TagType::LISTITEM:
        absl::StrAppend(&md_, "+ ");
        break;
      default:
        break;
    }
    return true;
  }

  bool VisitExit(const tinyxml2::XMLElement& element) override {
    switch (GetType(element)) {
      case TagType::CODE:
        absl::StrAppend(&md_, "`");
        break;
      case TagType::LINK:
        absl::StrAppend(&md_, "](", ReadAttribute(&element, "linkend"), ")");
        break;
      case TagType::LIST:
      case TagType::LISTITEM:
        absl::StrAppend(&md_, "\n");
        break;
      case TagType::PARAGRAPH:
        absl::StrAppend(&md_, "\n\n");
        break;
      default:
        break;
    }
    absl::StrAppend(&md_, " ");
    return true;
  }

  bool Visit(const XMLText& text) override {
    std::string value = text.Value();
    std::replace(value.begin(), value.end(), '\n', ' ');
    absl::StrAppend(&md_, value);
    return true;
  }

  const std::string& markdown() { return md_; }

 private:
  std::string md_;
};

}  // namespace

std::string ExportToMarkdown(const XMLElement* element) {
  if (!element) return "";

  TinyMarkdownParser parser;
  element->Accept(&parser);
  std::string md = parser.markdown();

  // Remove all non-newline whitespace before/after all newlines.
  RE2::GlobalReplace(&md, R"([^\S\n]*\n[^\S\n]*)", "\n");
  // Any remaining whitespace becomes a single space.
  RE2::GlobalReplace(&md, R"([^\S\n]+)", " ");
  // Condense 3+ newlines into 2.
  RE2::GlobalReplace(&md, R"(\n{3,})", "\n\n");
  // Remove leading and trailing whitespace.
  RE2::GlobalReplace(&md, R"(^\s+|\s+$)", "");

  return md;
}

}  // namespace xml
}  // namespace arm
}  // namespace exegesis
