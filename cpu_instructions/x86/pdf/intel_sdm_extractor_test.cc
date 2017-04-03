// Copyright 2016 Google Inc.
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

#include "cpu_instructions/x86/pdf/intel_sdm_extractor.h"

#include "cpu_instructions/base/pdf/pdf_document_parser.h"
#include "cpu_instructions/testing/test_util.h"
#include "cpu_instructions/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "strings/str_cat.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {
namespace {

using ::cpu_instructions::testing::EqualsProto;

const char kTestDataPath[] = "/__main__/cpu_instructions/x86/pdf/testdata/";

template <typename Proto>
Proto GetProto(const string& name) {
  return ReadTextProtoOrDie<Proto>(
      StrCat(getenv("TEST_SRCDIR"), kTestDataPath, name, ".pbtxt"));
}

TEST(IntelSdmExtractorTest, BitSetPage) {
  PdfDocument pdf_document = GetProto<PdfDocument>("253666_p170_p171_pdfdoc");
  for (auto& page : *pdf_document.mutable_pages()) {
    Cluster(&page);
  }

  const SdmDocument sdm_document =
      ConvertPdfDocumentToSdmDocument(pdf_document);
  EXPECT_THAT(sdm_document,
              EqualsProto(GetProto<SdmDocument>("253666_p170_p171_sdmdoc")));

  const InstructionSetProto instruction_set =
      ProcessIntelSdmDocument(sdm_document);
  EXPECT_THAT(instruction_set, EqualsProto(GetProto<InstructionSetProto>(
                                   "253666_p170_p171_instructionset")));
}

TEST(IntelSdmExtractorTest, ParseOperandEncodingTableCell) {
  EXPECT_THAT(ParseOperandEncodingTableCell("NA"), EqualsProto("spec: OE_NA"));

  EXPECT_THAT(ParseOperandEncodingTableCell("imm8"),
              EqualsProto("spec: OE_IMMEDIATE usage: USAGE_READ"));

  EXPECT_THAT(ParseOperandEncodingTableCell("imm8[7:4]"),
              EqualsProto("spec: OE_VEX_SUFFIX"));

  EXPECT_THAT(ParseOperandEncodingTableCell("3"),
              EqualsProto("spec: OE_CONSTANT usage: USAGE_READ"));

  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:r/m (r)"),
              EqualsProto("spec: OE_MOD_RM usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:r/m (w)"),
              EqualsProto("spec: OE_MOD_RM usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:r/m (r, w)"),
              EqualsProto("spec: OE_MOD_RM usage: USAGE_READ_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:rm (r)"),
              EqualsProto("spec: OE_MOD_RM usage: USAGE_READ"));
  EXPECT_THAT(
      ParseOperandEncodingTableCell("ModRM:r/m (r, ModRM:[7:6] must be 11b)"),
      EqualsProto("spec: OE_MOD_RM usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell(
                  "ModRM:r/m (w, ModRM:[7:6] must not be 11b)"),
              EqualsProto("spec: OE_MOD_RM usage: USAGE_WRITE"));

  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:reg (r)"),
              EqualsProto("spec: OE_MOD_REG usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:reg (w)"),
              EqualsProto("spec: OE_MOD_REG usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("ModRM:reg (r, w)"),
              EqualsProto("spec: OE_MOD_REG usage: USAGE_READ_WRITE"));

  EXPECT_THAT(ParseOperandEncodingTableCell("AX/EAX/RAX (r)"),
              EqualsProto("spec: OE_REGISTERS usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("AX/EAX/RAX (w)"),
              EqualsProto("spec: OE_REGISTERS usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("AX/EAX/RAX (r, w)"),
              EqualsProto("spec: OE_REGISTERS usage: USAGE_READ_WRITE"));

  EXPECT_THAT(ParseOperandEncodingTableCell("opcode + rd (r)"),
              EqualsProto("spec: OE_OPCODE usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("opcode + rd (w)"),
              EqualsProto("spec: OE_OPCODE usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("opcode + rd (r, w)"),
              EqualsProto("spec: OE_OPCODE usage: USAGE_READ_WRITE"));

  EXPECT_THAT(ParseOperandEncodingTableCell("VEX.vvvv (r)"),
              EqualsProto("spec: OE_VEX usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("VEX.vvvv (w)"),
              EqualsProto("spec: OE_VEX usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("VEX.vvvv (r, w)"),
              EqualsProto("spec: OE_VEX usage: USAGE_READ_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("VEX.1vvv (r)"),
              EqualsProto("spec: OE_VEX usage: USAGE_READ"));

  EXPECT_THAT(ParseOperandEncodingTableCell("EVEX.vvvv (r)"),
              EqualsProto("spec: OE_EVEX_V usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("EVEX.vvvv (w)"),
              EqualsProto("spec: OE_EVEX_V usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("EVEX.vvvv (r, w)"),
              EqualsProto("spec: OE_EVEX_V usage: USAGE_READ_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("vvvv (r)"),
              EqualsProto("spec: OE_EVEX_V usage: USAGE_READ"));

  EXPECT_THAT(ParseOperandEncodingTableCell("Implicit XMM0 (r)"),
              EqualsProto("spec: OE_IMPLICIT usage: USAGE_READ"));
  EXPECT_THAT(ParseOperandEncodingTableCell("Implicit XMM0 (w)"),
              EqualsProto("spec: OE_IMPLICIT usage: USAGE_WRITE"));
  EXPECT_THAT(ParseOperandEncodingTableCell("Implicit XMM0 (r, w)"),
              EqualsProto("spec: OE_IMPLICIT usage: USAGE_READ_WRITE"));

  EXPECT_THAT(
      ParseOperandEncodingTableCell("RDX/EDX is implied 64/32 bits \nsource"),
      EqualsProto("spec: OE_REGISTERS2"));

  EXPECT_THAT(ParseOperandEncodingTableCell(
                  "SIB.base (r): Address of pointer\nSIB.index(r)"),
              EqualsProto("spec: OE_SIB usage: USAGE_READ"));

  EXPECT_THAT(ParseOperandEncodingTableCell(
                  "BaseReg (R): VSIB:base,\nVectorReg(R): VSIB:index"),
              EqualsProto("spec: OE_VSIB usage: USAGE_READ"));
}
}  // namespace
}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
