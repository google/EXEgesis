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

#include <string>

#include "gtest/gtest.h"
#include "tinyxml2.h"

namespace exegesis {
namespace arm {
namespace xml {
namespace {

using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;

void ParseXML(const std::string& xml, XMLDocument* doc) {
  ASSERT_EQ(doc->Parse(xml.data(), xml.size()), ::tinyxml2::XML_SUCCESS);
}

TEST(MarkdownTest, Null) { EXPECT_EQ(ExportToMarkdown(nullptr), ""); }

TEST(MarkdownTest, ExportToMarkdown) {
  XMLDocument doc;
  ParseXML(R"(
      <root>
        <para>Paragraph containing <unknown>preserved</unknown>
          <complex attr="a">content</complex></para>
        <para>Another para with <xref linkend="target">label</xref></para>
        <list type="unordered">
          <listitem>
            <content>
              <instruction>NOP</instruction> semantics and
              syntax: <syntax>NOP;</syntax>.
            </content>
          </listitem>
          <listitem>
            Random <arm-defined-word>stuff</arm-defined-word>
            set to <value>42</value> or <hexnumber>0x2A</hexnumber>
            or <binarynumber>101010</binarynumber>
          </listitem>
        </list>
        <image file="http://file" label="img" ignored="stuff"></image>
        <note><para>a note with a <xref linkend="x">link</xref></para></note>
        <table><tgroup cols="2">
          <thead>
            <row>
              <entry>Header</entry>
              <entry>&lt;H&gt;</entry>
              </row>
          </thead>
          <tbody>
            <row>
              <entry>A</entry>
              <entry><syntax>0</syntax></entry>
            </row>
            <row>
              <entry><value>table</value></entry>
              <entry>
                <arch_variants><arch_variant feature="arm_id"/></arch_variants>
              </entry>
            </row>
          </tbody>
        </tgroup></table>
        <para>Yet <param>another</param> paragraph</para>
      </root>
      )",
           &doc);
  const XMLElement* element = doc.RootElement();

  EXPECT_EQ(ExportToMarkdown(element),
            "Paragraph containing preserved content\n"
            "\n"
            "Another para with [label](target)\n"
            "\n"
            "+ `NOP` semantics and syntax: `NOP;` .\n"
            "+ Random `stuff` set to `42` or `0x2A` or `101010`\n"
            "\n"
            "![img](http://file)\n"
            "\n"
            "> a note with a [link](x)\n"
            "\n"
            "| Header | <H> |\n"
            "| --- | --- |\n"
            "| A | `0` |\n"
            "| `table` | arm_id |\n"
            "\n"
            "Yet `another` paragraph");
}

}  // namespace
}  // namespace xml
}  // namespace arm
}  // namespace exegesis
