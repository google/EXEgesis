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

#include "exegesis/arm/xml/converter.h"

#include "exegesis/arm/xml/parser.pb.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace arm {
namespace xml {
namespace {

using ::exegesis::testing::EqualsProto;
using ::google::protobuf::TextFormat;

TEST(ConverterTest, ConvertToArchitectureProto) {
  static constexpr char kXmlDatabase[] = R"proto(
    base_index {
      files {
        filename: "instruction_1.xml"
        heading: "I1"
        xml_id: "id_i_1"
        description: "First instruction group."
      }
      files {
        filename: "instruction_2.xml"
        heading: "I2"
        xml_id: "id_i_2"
        description: "Second instruction group."
      }
    }
    fp_simd_index {
      files {
        filename: "instruction_3.xml"
        heading: "I3"
        xml_id: "id_i_3"
        description: "Third instruction group."
      }
    }
    instructions {
      xml_id: "id_i_1"
      heading: "I1 Title"
      brief_description: "First instruction"
      authored_description: "This is the first description"
      docvars { mnemonic: "I1" isa: A64 }
      classes {
        id: "class_1"
        name: "Class One"
        docvars { mnemonic: "I1" instr_class: GENERAL isa: A64 }
        encodings {
          id: "I1_class_1_enc_1_id"
          name: "I1_class_1_enc_1_name"
          docvars {
            mnemonic: "I1"
            cond_setting: S
            instr_class: GENERAL
            isa: A64
          }
          instruction_layout {
            form_name: "ps1a"
            bit_ranges {
              name: "msb"
              msb: 31
              lsb: 31
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
          id: "I1_class_1_enc_2_id"
          name: "I1_class_1_enc_2_name"
          docvars {
            mnemonic: "I1"
            cond_setting: NO_S
            instr_class: GENERAL
            isa: A64
          }
          instruction_layout {
            form_name: "ps1b"
            bit_ranges {
              name: "msb"
              msb: 31
              lsb: 31
              pattern { bits: CONSTANT_ONE }
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
      authored_description: "This is the second description"
      docvars { mnemonic: "I2" alias_mnemonic: "I1" isa: A64 }
      classes {
        id: "class_1"
        name: "Class One"
        docvars { mnemonic: "I2" isa: A64 }
        encodings {
          id: "I2_class_1_enc_1_id"
          name: "I2_class_1_enc_1_name"
          docvars { mnemonic: "I22" isa: A64 feature: CRC }
          instruction_layout {
            form_name: "ps2"
            bit_ranges {
              msb: 31
              lsb: 29
              not_pattern {
                bits: CONSTANT_ONE
                bits: CONSTANT_ZERO
                bits: VARIABLE
              }
            }
          }
          asm_template { pieces { text: "I2 " } }
        }
      }
    }
    instructions {
      xml_id: "id_i_3"
      heading: "I3 Title"
      brief_description: "Third instruction"
      authored_description: "This is the third description"
      docvars { cond_setting: S isa: A32 }
      classes {
        id: "class_1"
        name: "Class One"
        docvars { mnemonic: "I3" cond_setting: S isa: A32 }
        encodings {
          id: "I3_class_1_enc_1_id"
          name: "I3_class_1_enc_1_name"
          docvars { mnemonic: "I3" cond_setting: S isa: A32 }
        }
      }
    })proto";
  XmlDatabase xml_database;
  ASSERT_TRUE(TextFormat::ParseFromString(kXmlDatabase, &xml_database));

  static constexpr char kExpectedArchitecture[] = R"proto(
    name: 'ARMv8'
    instruction_set {
      source_infos { source_name: "ARM XML Database" }
      instructions {
        description: "First instruction | Class One | I1_class_1_enc_1_name"
        vendor_syntax {
          mnemonic: "I1"
          operands {
            name: "<range>"
            description: "Definition introduction, encoded in `range`\n\n"
                         "| range | Meaning |\n"
                         "| --- | --- |\n"
                         "| 010 | A |\n"
                         "| 110 | B |\n\n"
                         "More comments"
          }
          operands {
            name: "<bit>"
            description: "Is a 1-bit value, encoded in the `msb` field."
          }
        }
        available_in_64_bit: true
        encoding_scheme: "ps1a"
        fixed_size_encoding_specification {
          form_name: "ps1a"
          bit_ranges {
            name: "msb"
            msb: 31
            lsb: 31
            pattern { bits: CONSTANT_ZERO }
          }
        }
      }
      instructions {
        description: "First instruction | Class One | I1_class_1_enc_2_name"
        vendor_syntax {
          mnemonic: "I1"
          operands { name: "<range>" }
          operands {
            name: "<bit>"
            description: "Is a 1-bit value, encoded in the `msb` field."
          }
        }
        available_in_64_bit: true
        encoding_scheme: "ps1b"
        fixed_size_encoding_specification {
          form_name: "ps1b"
          bit_ranges {
            name: "msb"
            msb: 31
            lsb: 31
            pattern { bits: CONSTANT_ONE }
          }
        }
      }
      instructions {
        description: "Second instruction | Class One | I2_class_1_enc_1_name"
        vendor_syntax { mnemonic: "I22" }
        feature_name: "crc"
        available_in_64_bit: true
        encoding_scheme: "ps2"
        fixed_size_encoding_specification {
          form_name: "ps2"
          bit_ranges {
            msb: 31
            lsb: 29
            not_pattern {
              bits: CONSTANT_ONE
              bits: CONSTANT_ZERO
              bits: VARIABLE
            }
          }
        }
        instruction_group_index: 1
      }
      instructions {
        description: "Third instruction | Class One | I3_class_1_enc_1_name"
        vendor_syntax { mnemonic: "I3" }
        available_in_64_bit: false
        fixed_size_encoding_specification {}
        instruction_group_index: 2
      }
      instruction_groups {
        name: "I1"
        description: "This is the first description"
        short_description: "First instruction group."
      }
      instruction_groups {
        name: "I2"
        description: "This is the second description"
        short_description: "Second instruction group."
      }
      instruction_groups {
        name: "I3"
        description: "This is the third description"
        flags_affected { content: "S" }
        short_description: "Third instruction group."
      }
    })proto";
  EXPECT_THAT(ConvertToArchitectureProto(xml_database),
              EqualsProto(kExpectedArchitecture));
}

}  // namespace
}  // namespace xml
}  // namespace arm
}  // namespace exegesis
