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

#include "exegesis/arm/xml/parser.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace arm {
namespace xml {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::testing::IsOkAndHolds;
using ::exegesis::testing::StatusIs;
using ::exegesis::util::error::INTERNAL;
using ::testing::HasSubstr;

const char kTestDataPath[] = "/__main__/exegesis/arm/xml/testdata/";

std::string GetFilePath(const std::string& filename) {
  return absl::StrCat(getenv("TEST_SRCDIR"), kTestDataPath, filename);
}

TEST(ParserTest, ParseXmlIndex) {
  static constexpr char kExpectedIndex[] = R"proto(
    isa: A64
    files {
      filename: "instruction_1.xml"
      heading: "I1"
      xml_id: "id_i_1"
      description: "First instruction."
    }
    files {
      filename: "instruction_2.xml"
      heading: "I2"
      xml_id: "id_i_2"
      description: "Second instruction."
    })proto";
  EXPECT_THAT(ParseXmlIndex(GetFilePath("index.xml")),
              IsOkAndHolds(EqualsProto(kExpectedIndex)));

  static constexpr char kExpectedFpsimdindex[] = R"( isa: A64 )";
  EXPECT_THAT(ParseXmlIndex(GetFilePath("fpsimdindex.xml")),
              IsOkAndHolds(EqualsProto(kExpectedFpsimdindex)));
}

TEST(ParserTest, ParseXmlIndex_Missing) {
  EXPECT_THAT(ParseXmlIndex(GetFilePath("missing.xml")),
              StatusIs(INTERNAL, HasSubstr("XML_ERROR_FILE_NOT_FOUND")));
}

TEST(ParserTest, ParseXmlInstruction) {
  static constexpr char kExpectedInstruction[] = R"proto(
    xml_id: "id_i_1"
    heading: "I1 Title"
    brief_description: "First instruction"
    authored_description: "This is the first instruction"
    docvars { mnemonic: "I1" isa: A64 }
    classes {
      id: "class_1"
      name: "Class One"
      docvars { mnemonic: "I1" instr_class: GENERAL isa: A64 }
      encodings {
        id: "I1_class_1_enc_1"
        name: "First encoding of Class One (bit == 0)"
        docvars { mnemonic: "I1" cond_setting: S instr_class: GENERAL isa: A64 }
        instruction_layout {
          form_name: "ps"
          bit_ranges {
            name: "msb"
            msb: 31
            lsb: 31
            pattern { bits: CONSTANT_ZERO }
          }
          bit_ranges {
            name: "range"
            msb: 30
            lsb: 28
            pattern { bits: VARIABLE bits: CONSTANT_ONE bits: CONSTANT_ZERO }
          }
          bit_ranges {
            name: "partial-override"
            msb: 2
            lsb: 1
            pattern { bits: CONSTANT_ZERO bits: CONSTANT_ONE }
          }
          bit_ranges {
            name: "lsb"
            pattern { bits: CONSTANT_ZERO }
          }
        }
        asm_template {
          pieces { text: "I1 " }
          pieces {
            symbol {
              id: "range"
              label: "<range>"
              encoded_in: "range"
              description: "range short description 1"
              explanation: "Definition introduction, encoded in `range`\n\n"
                           "| range | Meaning |\n"
                           "| --- | --- |\n"
                           "| 010 | A |\n"
                           "| 110 | B |\n\n"
                           "More comments"
            }
          }
          pieces { text: ", " }
          pieces {
            symbol {
              id: "bit"
              label: "<bit>"
              encoded_in: "msb"
              description: "bit short description"
              explanation: "Is a 1-bit value, encoded in the `msb` field."
            }
          }
          pieces { text: ", 0" }
        }
      }
      encodings {
        id: "I1_class_1_enc_2"
        name: "Second encoding of Class One"
        docvars {
          mnemonic: "I1"
          cond_setting: NO_S
          instr_class: GENERAL
          isa: A64
        }
        instruction_layout {
          form_name: "ps"
          bit_ranges {
            name: "msb"
            msb: 31
            lsb: 31
            pattern { bits: CONSTANT_ONE }
          }
          bit_ranges {
            name: "range"
            msb: 30
            lsb: 28
            pattern { bits: VARIABLE bits: CONSTANT_ONE bits: CONSTANT_ZERO }
          }
          bit_ranges {
            name: "partial-override"
            msb: 2
            lsb: 1
            pattern { bits: CONSTANT_ZERO bits: VARIABLE }
          }
          bit_ranges {
            name: "lsb"
            pattern { bits: CONSTANT_ZERO }
          }
        }
        asm_template {
          pieces { text: "I1 " }
          pieces {
            symbol {
              id: "range"
              label: "<range>"
              description: "range short description 2"
            }
          }
          pieces { text: ", " }
          pieces {
            symbol {
              id: "bit"
              label: "<bit>"
              encoded_in: "msb"
              description: "bit short description"
              explanation: "Is a 1-bit value, encoded in the `msb` field."
            }
          }
          pieces { text: ", 1" }
        }
      }
    })proto";
  EXPECT_THAT(ParseXmlInstruction(GetFilePath("instruction_1.xml")),
              IsOkAndHolds(EqualsProto(kExpectedInstruction)));
}

TEST(ParserTest, ParseXmlInstructionWithConstraints) {
  static constexpr char kExpectedInstruction[] = R"proto(
    xml_id: "id_i_2"
    heading: "I2 Title"
    brief_description: "Second instruction"
    authored_description: "This is the second instruction"
    docvars { mnemonic: "I2" isa: A64 }
    classes {
      id: "class_1"
      name: "Class One"
      docvars { mnemonic: "I2" isa: A64 }
      encodings {
        id: "I2_class_1_enc_1"
        name: "First encoding of Class One"
        docvars { mnemonic: "I2" isa: A64 }
        instruction_layout {
          form_name: "ps"
          bit_ranges {
            msb: 31
            lsb: 25
            not_pattern {
              bits: CONSTANT_ONE
              bits: CONSTANT_ZERO
              bits: CONSTANT_ONE
              bits: CONSTANT_ONE
              bits: CONSTANT_ZERO
              bits: VARIABLE
              bits: VARIABLE
            }
          }
          bit_ranges {
            name: "lsb"
            pattern { bits: CONSTANT_ZERO }
          }
        }
        asm_template { pieces { text: "I2 " } }
      }
    })proto";
  EXPECT_THAT(ParseXmlInstruction(GetFilePath("instruction_2.xml")),
              IsOkAndHolds(EqualsProto(kExpectedInstruction)));
}

TEST(ParserTest, ParseXmlInstruction_Missing) {
  EXPECT_THAT(ParseXmlInstruction(GetFilePath("missing.xml")),
              StatusIs(INTERNAL, HasSubstr("XML_ERROR_FILE_NOT_FOUND")));
}

TEST(ParserTest, ParseXmlDatabase) {
  static constexpr char kExpectedXmlDatabase[] = R"proto(
    base_index {
      isa: A64
      files {
        filename: "instruction_1.xml"
        heading: "I1"
        xml_id: "id_i_1"
        description: "First instruction."
      }
      files {
        filename: "instruction_2.xml"
        heading: "I2"
        xml_id: "id_i_2"
        description: "Second instruction."
      }
    }
    fp_simd_index { isa: A64 }
    instructions {
      xml_id: "id_i_1"
      heading: "I1 Title"
      brief_description: "First instruction"
      authored_description: "This is the first instruction"
      docvars { mnemonic: "I1" isa: A64 }
      classes {
        id: "class_1"
        name: "Class One"
        docvars { mnemonic: "I1" instr_class: GENERAL isa: A64 }
        encodings {
          id: "I1_class_1_enc_1"
          name: "First encoding of Class One (bit == 0)"
          docvars {
            mnemonic: "I1"
            cond_setting: S
            instr_class: GENERAL
            isa: A64
          }
          instruction_layout {
            form_name: "ps"
            bit_ranges {
              name: "msb"
              msb: 31
              lsb: 31
              pattern { bits: CONSTANT_ZERO }
            }
            bit_ranges {
              name: "range"
              msb: 30
              lsb: 28
              pattern { bits: VARIABLE bits: CONSTANT_ONE bits: CONSTANT_ZERO }
            }
            bit_ranges {
              name: "partial-override"
              msb: 2
              lsb: 1
              pattern { bits: CONSTANT_ZERO bits: CONSTANT_ONE }
            }
            bit_ranges {
              name: "lsb"
              pattern { bits: CONSTANT_ZERO }
            }
          }
          asm_template {
            pieces { text: "I1 " }
            pieces {
              symbol {
                id: "range"
                label: "<range>"
                encoded_in: "range"
                description: "range short description 1"
                explanation: "Definition introduction, encoded in `range`\n\n"
                             "| range | Meaning |\n"
                             "| --- | --- |\n"
                             "| 010 | A |\n"
                             "| 110 | B |\n\n"
                             "More comments"
              }
            }
            pieces { text: ", " }
            pieces {
              symbol {
                id: "bit"
                label: "<bit>"
                encoded_in: "msb"
                description: "bit short description"
                explanation: "Is a 1-bit value, encoded in the `msb` field."
              }
            }
            pieces { text: ", 0" }
          }
        }
        encodings {
          id: "I1_class_1_enc_2"
          name: "Second encoding of Class One"
          docvars {
            mnemonic: "I1"
            cond_setting: NO_S
            instr_class: GENERAL
            isa: A64
          }
          instruction_layout {
            form_name: "ps"
            bit_ranges {
              name: "msb"
              msb: 31
              lsb: 31
              pattern { bits: CONSTANT_ONE }
            }
            bit_ranges {
              name: "range"
              msb: 30
              lsb: 28
              pattern { bits: VARIABLE bits: CONSTANT_ONE bits: CONSTANT_ZERO }
            }
            bit_ranges {
              name: "partial-override"
              msb: 2
              lsb: 1
              pattern { bits: CONSTANT_ZERO bits: VARIABLE }
            }
            bit_ranges {
              name: "lsb"
              pattern { bits: CONSTANT_ZERO }
            }
          }
          asm_template {
            pieces { text: "I1 " }
            pieces {
              symbol {
                id: "range"
                label: "<range>"
                description: "range short description 2"
              }
            }
            pieces { text: ", " }
            pieces {
              symbol {
                id: "bit"
                label: "<bit>"
                encoded_in: "msb"
                description: "bit short description"
                explanation: "Is a 1-bit value, encoded in the `msb` field."
              }
            }
            pieces { text: ", 1" }
          }
        }
      }
    }
    instructions {
      xml_id: "id_i_2"
      heading: "I2 Title"
      brief_description: "Second instruction"
      authored_description: "This is the second instruction"
      docvars { mnemonic: "I2" isa: A64 }
      classes {
        id: "class_1"
        name: "Class One"
        docvars { mnemonic: "I2" isa: A64 }
        encodings {
          id: "I2_class_1_enc_1"
          name: "First encoding of Class One"
          docvars { mnemonic: "I2" isa: A64 }
          instruction_layout {
            form_name: "ps"
            bit_ranges {
              msb: 31
              lsb: 25
              not_pattern {
                bits: CONSTANT_ONE
                bits: CONSTANT_ZERO
                bits: CONSTANT_ONE
                bits: CONSTANT_ONE
                bits: CONSTANT_ZERO
                bits: VARIABLE
                bits: VARIABLE
              }
            }
            bit_ranges {
              name: "lsb"
              pattern { bits: CONSTANT_ZERO }
            }
          }
          asm_template { pieces { text: "I2 " } }
        }
      }
    })proto";
  EXPECT_THAT(ParseXmlDatabase(GetFilePath("")),
              IsOkAndHolds(EqualsProto(kExpectedXmlDatabase)));
}

}  // namespace
}  // namespace xml
}  // namespace arm
}  // namespace exegesis
