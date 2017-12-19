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

#include "exegesis/arm/xml/docvars.h"

#include <string>

#include "exegesis/arm/xml/docvars.pb.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tinyxml2.h"

namespace exegesis {
namespace arm {
namespace xml {
namespace {

using ::exegesis::arm::xml::dv::DocVars;
using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::IsOk;
using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::FAILED_PRECONDITION;
using ::exegesis::util::error::UNIMPLEMENTED;
using ::testing::HasSubstr;
using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;

void ParseXML(const std::string& xml, XMLDocument* doc) {
  ASSERT_EQ(doc->Parse(xml.data(), xml.size()), ::tinyxml2::XML_SUCCESS);
}

TEST(DocVarsTest, ParseDocVars) {
  XMLDocument doc;
  ParseXML(R"(
      <docvars>
        <docvar key="mnemonic" value="LDADD" />
        <docvar key="alias_mnemonic" value="STADD" />
        <docvar key="" value="ignored" />
        <docvar nokey="something" value="ignored again" />
        <docvar key="bitfield-fill" value="signed-fill" />
        <docvar key="instr-class" value="general" />
        <docvar key="isa" value="A64" />
        <docvar key="atomic-ops" value="LDADD-32-reg" />
        <docvar key="reg-type" value="32-reg" />
        <not_a_docvar key="address-form" value="literal" />
        <docvar key="source-type" value="src-is-immediate" />
      </docvars>
      )",
           &doc);
  XMLElement* element = doc.RootElement();

  DocVars expected;
  expected.set_mnemonic("LDADD");
  expected.set_alias_mnemonic("STADD");
  expected.set_atomic_ops(dv::AtomicOps::ATOMIC_OPS_SET);
  expected.set_bitfield_fill(dv::BitfieldFill::SIGNED_FILL);
  expected.set_instr_class(dv::InstrClass::GENERAL);
  expected.set_isa(dv::Isa::A64);
  expected.set_reg_type(dv::RegType::X__32_REG);
  expected.set_source_type(dv::SourceType::SRC_IS_IMMEDIATE);
  EXPECT_THAT(ParseDocVars(element), IsOkAndHolds(EqualsProto(expected)));
}

TEST(DocVarsTest, ParseDocVars_UnimplementedKey) {
  XMLDocument doc;
  ParseXML(R"(
      <docvars>
        <docvar key="isa" value="A64" />
        <docvar key="bad-key" value="value" />
      </docvars>
      )",
           &doc);
  XMLElement* element = doc.RootElement();

  EXPECT_THAT(
      ParseDocVars(element),
      StatusIs(UNIMPLEMENTED, HasSubstr("Unknown docvar key 'bad-key'")));
}

TEST(DocVarsTest, ParseDocVars_UnimplementedValue) {
  XMLDocument doc;
  ParseXML(R"(
      <docvars>
        <docvar key="isa" value="A64" />
        <docvar key="reg-type" value="bad-value" />
      </docvars>
      )",
           &doc);
  XMLElement* element = doc.RootElement();

  EXPECT_THAT(
      ParseDocVars(element),
      StatusIs(UNIMPLEMENTED, HasSubstr("Bad value 'bad-value' for RegType")));
}

TEST(DocVarsTest, DocVarsContains) {
  const DocVars empty;

  DocVars subset;
  subset.set_mnemonic("mnemonic");
  subset.set_address_form(dv::AddressForm::LITERAL);
  subset.set_isa(dv::Isa::A64);
  subset.set_instr_class(dv::InstrClass::UNKNOWN_INSTR_CLASS);

  DocVars superset = subset;
  superset.set_alias_mnemonic("alias_mnemonic");
  superset.set_datatype(dv::Datatype::SINGLE);
  superset.set_reg_type(dv::RegType::PAIR_DOUBLEWORDS);

  EXPECT_THAT(DocVarsContains(empty, empty), IsOk());
  EXPECT_THAT(DocVarsContains(subset, empty), IsOk());
  EXPECT_THAT(DocVarsContains(superset, empty), IsOk());
  EXPECT_THAT(DocVarsContains(subset, subset), IsOk());
  EXPECT_THAT(DocVarsContains(superset, subset), IsOk());
  EXPECT_THAT(DocVarsContains(superset, superset), IsOk());

  EXPECT_THAT(DocVarsContains(empty, subset), StatusIs(FAILED_PRECONDITION));
  EXPECT_THAT(DocVarsContains(empty, superset), StatusIs(FAILED_PRECONDITION));
  EXPECT_THAT(DocVarsContains(subset, superset), StatusIs(FAILED_PRECONDITION));

  // Introduce a field with a different value.
  subset.set_datatype(dv::Datatype::DOUBLE);
  EXPECT_THAT(DocVarsContains(superset, subset),
              StatusIs(FAILED_PRECONDITION,
                       HasSubstr("modified: datatype: DOUBLE -> SINGLE")));
}

}  // namespace
}  // namespace xml
}  // namespace arm
}  // namespace exegesis
