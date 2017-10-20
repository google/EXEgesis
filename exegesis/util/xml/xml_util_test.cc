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

#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tinyxml2.h"
#include "util/task/status.h"

namespace exegesis {
namespace xml {
namespace {

using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::INTERNAL;
using ::exegesis::util::error::NOT_FOUND;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Ne;
using ::testing::NotNull;
using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;
using ::tinyxml2::XMLError;

void ParseXML(const std::string& xml, XMLDocument* doc) {
  ASSERT_EQ(doc->Parse(xml.data(), xml.size()), ::tinyxml2::XML_SUCCESS);
}

TEST(XmlUtilTest, CheckResult_Ok) {
  EXPECT_OK(GetStatus(::tinyxml2::XML_SUCCESS));

  EXPECT_THAT(GetStatus(::tinyxml2::XML_NO_ATTRIBUTE),
              StatusIs(INTERNAL, HasSubstr("XML_NO_ATTRIBUTE")));
  EXPECT_THAT(GetStatus(::tinyxml2::XML_NO_TEXT_NODE),
              StatusIs(INTERNAL, HasSubstr("XML_NO_TEXT_NODE")));
}

TEST(XmlUtilTest, DebugString) {
  XMLDocument doc;
  EXPECT_EQ(DebugString(&doc), "");

  ParseXML("<doc>simple</doc>", &doc);
  EXPECT_EQ(DebugString(&doc), "<doc>simple</doc>\n");

  ParseXML("<doc>text<tag attr=\"2\">inside</tag></doc>", &doc);
  EXPECT_EQ(DebugString(&doc),
            "<doc>text    <tag attr=\"2\">inside</tag>\n</doc>\n");
}

TEST(XmlUtilTest, FindChild) {
  XMLDocument doc;

  EXPECT_THAT(FindChild(&doc, nullptr), StatusIs(NOT_FOUND));
  EXPECT_THAT(FindChild(&doc, "a"), StatusIs(NOT_FOUND));

  XMLElement* element_1 = doc.NewElement("a");
  XMLElement* element_2 = doc.NewElement("b");
  XMLElement* element_3 = doc.NewElement("c");
  XMLElement* element_4 = doc.NewElement("b");
  ASSERT_THAT(doc.InsertEndChild(element_1), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_2), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_3), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_4), NotNull());

  EXPECT_THAT(FindChild(&doc, nullptr), IsOkAndHolds(element_1));
  EXPECT_THAT(FindChild(&doc, "a"), IsOkAndHolds(element_1));
  EXPECT_THAT(FindChild(&doc, "b"), IsOkAndHolds(element_2));
  EXPECT_THAT(FindChild(&doc, "b"), IsOkAndHolds(Ne(element_4)));
  EXPECT_THAT(FindChild(&doc, "c"), IsOkAndHolds(element_3));
  EXPECT_THAT(FindChild(&doc, "d"), StatusIs(NOT_FOUND));
}

TEST(XmlUtilTest, FindChildren) {
  XMLDocument doc;

  EXPECT_THAT(FindChildren(&doc, nullptr), IsEmpty());
  EXPECT_THAT(FindChildren(&doc, "a"), IsEmpty());

  XMLElement* element_1 = doc.NewElement("a");
  XMLElement* element_2 = doc.NewElement("a");
  XMLElement* element_3 = doc.NewElement("b");
  XMLElement* element_4 = doc.NewElement("a");
  ASSERT_THAT(doc.InsertEndChild(element_1), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_2), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_3), NotNull());
  ASSERT_THAT(doc.InsertEndChild(element_4), NotNull());

  EXPECT_THAT(FindChildren(&doc, nullptr),
              ElementsAre(element_1, element_2, element_3, element_4));
  EXPECT_THAT(FindChildren(&doc, "a"),
              ElementsAre(element_1, element_2, element_4));
  EXPECT_THAT(FindChildren(&doc, "b"), ElementsAre(element_3));
  EXPECT_THAT(FindChildren(&doc, "c"), IsEmpty());
}

TEST(XmlUtilTest, ReadAttribute) {
  XMLDocument doc;
  ParseXML(R"(<doc key="value" empty="" />)", &doc);
  XMLElement* element = doc.RootElement();

  EXPECT_EQ(ReadAttribute(element, "key"), "value");
  EXPECT_EQ(ReadAttribute(element, "empty"), "");
  EXPECT_EQ(ReadAttribute(element, "absent"), "");
}

TEST(XmlUtilTest, ReadIntAttribute) {
  XMLDocument doc;
  ParseXML(R"(<doc int="42" neg="-3" fp="73.92" text="NaN" empty="" />)", &doc);
  XMLElement* element = doc.RootElement();

  EXPECT_THAT(ReadIntAttribute(element, "int"), IsOkAndHolds(42));
  EXPECT_THAT(ReadIntAttribute(element, "neg"), IsOkAndHolds(-3));
  EXPECT_THAT(ReadIntAttribute(element, "fp"), IsOkAndHolds(73));
  EXPECT_THAT(ReadIntAttribute(element, "text"), StatusIs(INTERNAL));
  EXPECT_THAT(ReadIntAttribute(element, "empty"), StatusIs(INTERNAL));
  EXPECT_THAT(ReadIntAttribute(element, "absent"), StatusIs(INTERNAL));
}

TEST(XmlUtilTest, ReadIntAttributeOrDefault) {
  XMLDocument doc;
  ParseXML(R"(<doc int="42" neg="-3" fp="73.92" text="NaN" empty="" />)", &doc);
  XMLElement* element = doc.RootElement();

  EXPECT_EQ(ReadIntAttributeOrDefault(element, "int", 101), 42);
  EXPECT_EQ(ReadIntAttributeOrDefault(element, "neg", 101), -3);
  EXPECT_EQ(ReadIntAttributeOrDefault(element, "fp", 101), 73);
  EXPECT_EQ(ReadIntAttributeOrDefault(element, "text", 101), 101);
  EXPECT_EQ(ReadIntAttributeOrDefault(element, "empty", 101), 101);
  EXPECT_EQ(ReadIntAttributeOrDefault(element, "absent", 101), 101);
}

TEST(XmlUtilTest, ReadSimpleText) {
  XMLDocument doc;
  ParseXML(R"(
      <a></a>
      <b>basic text</b>
      <c>complex<tag>tree</tag></c>
      )",
           &doc);
  XMLElement* element_1 = doc.FirstChildElement("a");
  XMLElement* element_2 = doc.FirstChildElement("b");
  XMLElement* element_3 = doc.FirstChildElement("c");
  ASSERT_THAT(element_1, NotNull());
  ASSERT_THAT(element_2, NotNull());
  ASSERT_THAT(element_3, NotNull());

  EXPECT_EQ(ReadSimpleText(element_1), "");
  EXPECT_EQ(ReadSimpleText(element_2), "basic text");
  EXPECT_EQ(ReadSimpleText(element_3), "complex");
}

TEST(XmlUtilTest, ReadHtmlText) {
  XMLDocument doc;
  ParseXML(R"(
      <a></a>
      <b>basic text</b>
      <c>complex<tag>tree</tag></c>
      )",
           &doc);
  XMLElement* element_1 = doc.FirstChildElement("a");
  XMLElement* element_2 = doc.FirstChildElement("b");
  XMLElement* element_3 = doc.FirstChildElement("c");
  ASSERT_THAT(element_1, NotNull());
  ASSERT_THAT(element_2, NotNull());
  ASSERT_THAT(element_3, NotNull());

  EXPECT_EQ(ReadHtmlText(element_1), "<a/>");
  EXPECT_EQ(ReadHtmlText(element_2), "<b>basic text</b>");
  EXPECT_EQ(ReadHtmlText(element_3), "<c>complex<tag>tree</tag></c>");
}

}  // namespace
}  // namespace xml
}  // namespace exegesis
