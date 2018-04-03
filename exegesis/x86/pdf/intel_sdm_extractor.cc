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

#include "exegesis/x86/pdf/intel_sdm_extractor.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/text_processing.h"
#include "exegesis/x86/pdf/vendor_syntax.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace x86 {
namespace pdf {

namespace {

using exegesis::pdf::PdfDocument;
using exegesis::pdf::PdfPage;
using exegesis::pdf::PdfTextBlock;
using exegesis::pdf::PdfTextTableRow;

// The top/bottom page margin, in pixels.
constexpr const float kPageMargin = 50.0f;

// Returns the value associated to the first matching regexp. If there is a
// match, the function returns the first matching RE2 object from 'matchers';
// otherwise, it returns nullptr.
//
// The value type of the container must be std::pair<value, matcher> or other
// type that behaves the same way. Note that the following two containers
// satisfy the requirements: std::map<ValueType, RE2*> and
// std::vector<std::pair<ValueType, RE2*>>.
template <typename ValueType, typename Container>
const RE2* TryParse(const Container& matchers, const std::string& text,
                    ValueType* output) {
  CHECK(output != nullptr) << "must not be nullptr";
  for (const auto& pair : matchers) {
    if (RE2::FullMatch(text, *pair.second)) {
      *output = pair.first;
      return pair.second;
    }
  }
  return nullptr;
}

// Returns the value associated to the first matching regexp in the map or the
// provided default value.
template <typename ValueType, typename Container>
ValueType ParseWithDefault(const Container& matchers, const std::string& text,
                           const ValueType& default_value) {
  for (const auto& pair : matchers) {
    if (RE2::FullMatch(text, *pair.second)) return pair.first;
  }
  return default_value;
}

typedef std::vector<const PdfPage*> Pages;
typedef std::vector<const PdfTextTableRow*> Rows;
typedef google::protobuf::RepeatedField<InstructionTable::Column> Columns;

void RemoveAllChars(std::string* text, absl::string_view chars) {
  text->erase(std::remove_if(text->begin(), text->end(),
                             [chars](const char c) {
                               return chars.find(c) != absl::string_view::npos;
                             }),
              text->end());
}

void RemoveSpaceAndLF(std::string* text) { RemoveAllChars(text, "\n "); }

constexpr const size_t kMaxInstructionIdSize = 60;
constexpr const char kInstructionSetRef[] = "INSTRUCTION SET REFERENCE";

// This function is used to create a stable id from instruction name found:
// - at the top of a page describing a new instruction
// - in the footer of a page for a particular instruction
// It does so by removing some characters and imposing a limit on the text size.
// Limiting the size is necessary because when text is too long it gets
// truncated in different ways.
std::string Normalize(std::string text) {
  RemoveAllChars(&text, "\n ∗*");
  if (text.size() > kMaxInstructionIdSize) text.resize(kMaxInstructionIdSize);
  return text;
}

// If page number is even, returns the rightmost string in the footer, else the
// leftmost string.
const std::string& GetFooterSectionName(const PdfPage& page) {
  return page.number() % 2 == 0 ? GetCellTextOrEmpty(page, -1, -1)
                                : GetCellTextOrEmpty(page, -1, 0);
}

// If 'page' is the first page of an instruction, returns a unique identifier
// for this instruction. Otherwise return empty string.
std::string GetInstructionGroupId(const PdfPage& page) {
  static constexpr float kMaxGroupNameVerticalPosition = 500.0f;
  if (!absl::StartsWith(GetCellTextOrEmpty(page, 0, 0), kInstructionSetRef))
    return {};
  // We require that the name of the instruction group is in the top part of the
  // page. This prevents the parser from recognizing pages with too few elements
  // on them as instruction sections. This was the case for example with the
  // December 2017 version of the SDM on page 581 of the combined volumes PDF.
  const PdfTextBlock* const name_cell = GetCellOrNull(page, 1, 0);
  if (name_cell == nullptr ||
      name_cell->bounding_box().top() > kMaxGroupNameVerticalPosition) {
    return {};
  }
  const std::string maybe_instruction = Normalize(name_cell->text());
  const std::string& footer_section_name = GetFooterSectionName(page);
  if (maybe_instruction == Normalize(footer_section_name)) {
    return footer_section_name;
  }
  return {};
}

// True if page footer's corresponds to the same instruction_id.
bool IsPageInstruction(const PdfPage& page,
                       const std::string& instruction_group_id) {
  return Normalize(GetFooterSectionName(page)) ==
         Normalize(instruction_group_id);
}

// Returns the list of pages an instruction spans.
std::vector<const PdfPage*> GetInstructionsPages(
    const PdfDocument& document, const int first_page,
    const std::string& instruction_group_id) {
  std::vector<const PdfPage*> result;
  for (int i = first_page; i < document.pages_size(); ++i) {
    const auto& page = document.pages(i);
    if (!IsPageInstruction(page, instruction_group_id)) break;
    result.push_back(&page);
  }
  return result;
}

constexpr const float kMinSubSectionTitleFontSize = 9.5f;

std::string GetSubSectionTitle(const PdfTextTableRow& row) {
  if (row.blocks().empty() || row.blocks_size() > 2) return {};
  const auto& block = row.blocks(0);
  if (block.font_size() < kMinSubSectionTitleFontSize) return {};
  std::string text = block.text();
  absl::StripAsciiWhitespace(&text);
  if (absl::StartsWith(text, "Table") || absl::StartsWith(text, "Figure") ||
      absl::StartsWith(text, "Example"))
    return {};
  return text;
}

const std::map<SubSection::Type, const RE2*>& GetSubSectionMatchers() {
  static const auto* kSubSection = new std::map<SubSection::Type, const RE2*>{
      {SubSection::CPP_COMPILER_INTRISIC,
       new RE2(".*C/C\\+\\+ Compiler Intrinsic Equivalent.*")},
      {SubSection::DESCRIPTION, new RE2("Description")},
      {SubSection::EFFECTIVE_OPERAND_SIZE, new RE2("Effective Operand Size")},
      {SubSection::EXCEPTIONS, new RE2("Exceptions \\(All .*")},
      {SubSection::EXCEPTIONS_64BITS_MODE,
       new RE2("64-[Bb]it Mode Exceptions")},
      {SubSection::EXCEPTIONS_COMPATIBILITY_MODE,
       new RE2("Compatibility Mode Exceptions")},
      {SubSection::EXCEPTIONS_FLOATING_POINT,
       new RE2("Floating-Point Exceptions")},
      {SubSection::EXCEPTIONS_NUMERIC, new RE2("Numeric Exceptions")},
      {SubSection::EXCEPTIONS_OTHER, new RE2("Other Exceptions")},
      {SubSection::EXCEPTIONS_PROTECTED_MODE,
       new RE2("Protected Mode Exceptions")},
      {SubSection::EXCEPTIONS_REAL_ADDRESS_MODE,
       new RE2("Real[- ]Address Mode Exceptions")},
      {SubSection::EXCEPTIONS_VIRTUAL_8086_MODE,
       new RE2("Virtual[- ]8086 Mode Exceptions")},
      {SubSection::FLAGS_AFFECTED, new RE2("A?Flags Affected")},
      {SubSection::FLAGS_AFFECTED_FPU, new RE2("FPU Flags Affected")},
      {SubSection::FLAGS_AFFECTED_INTEGER, new RE2("Integer Flags Affected")},
      {SubSection::IA32_ARCHITECTURE_COMPATIBILITY,
       new RE2("IA-32 Architecture Compatibility")},
      {SubSection::IA32_ARCHITECTURE_LEGACY_COMPATIBILITY,
       new RE2("IA-32 Architecture Legacy Compatibility")},
      {SubSection::IMPLEMENTATION_NOTES, new RE2("Implementation Notes?")},
      {SubSection::INSTRUCTION_OPERAND_ENCODING,
       new RE2("Instruction Operand Encoding1?")},
      {SubSection::NOTES, new RE2("Notes:")},
      {SubSection::OPERATION, new RE2("Operation")},
      {SubSection::OPERATION_IA32_MODE, new RE2("IA-32e Mode Operation")},
      {SubSection::OPERATION_NON_64BITS_MODE,
       new RE2("Non-64-Bit Mode Operation")},
  };
  return *kSubSection;
}

const std::vector<std::pair<InstructionTable::Column, const RE2*>>&
GetInstructionColumnMatchers() {
  static const auto* kInstructionColumns =
      new std::vector<std::pair<InstructionTable::Column, const RE2*>>{
          {InstructionTable::IT_OPCODE, new RE2(R"(Opcode\*{0,3})")},
          {InstructionTable::IT_OPCODE_INSTRUCTION,
           new RE2(R"(Opcode ?\*?/? ?\n?Instruction)")},
          {InstructionTable::IT_INSTRUCTION, new RE2(R"(Instruction)")},
          {InstructionTable::IT_MODE_SUPPORT_64_32BIT,
           new RE2(R"(32/64 ?\nbit Mode ?\nSupport)")},
          {InstructionTable::IT_MODE_SUPPORT_64_32BIT,
           new RE2(R"(64/3\n?2\n?[- ]?\n?bit \n?Mode( \n?Support)?)")},
          {InstructionTable::IT_MODE_SUPPORT_64BIT,
           new RE2(R"(64-[Bb]it \n?Mode)")},
          {InstructionTable::IT_MODE_COMPAT_LEG,
           new RE2(R"(Compat/\n?Leg Mode\*?)")},
          {InstructionTable::IT_FEATURE_FLAG,
           new RE2(R"(CPUID(\ ?\n?Fea\-?\n?ture \n?Flag)?)")},  // NOTYPO
          {InstructionTable::IT_DESCRIPTION, new RE2(R"(Description)")},
          {InstructionTable::IT_OP_EN, new RE2(R"(Op\ ?\n?/?\ ?\n?E\n?[nN])")},
      };
  return *kInstructionColumns;
}

const std::map<InstructionTable::Mode, const RE2*>&
GetInstructionModeMatchers() {
  static const auto* kModes = new std::map<InstructionTable::Mode, const RE2*>{
      {InstructionTable::MODE_V, new RE2(R"([Vv](?:alid)?[1-9*]*)")},
      {InstructionTable::MODE_I, new RE2(R"(Inv\.|[Ii](?:nvalid)?[1-9*]*)")},
      {InstructionTable::MODE_NE, new RE2(R"(NA|NE|N\. ?E1?\.[1-9*]*)")},
      {InstructionTable::MODE_NP, new RE2(R"(NP)")},
      {InstructionTable::MODE_NI, new RE2(R"(NI)")},
      {InstructionTable::MODE_NS, new RE2(R"(N\.?S\.?)")},
  };
  return *kModes;
}

const std::set<absl::string_view>& GetValidFeatureSet() {
  static const auto* kValidFeatures = new std::set<absl::string_view>{
      "3DNOW",       "ADX",      "AES",           "AVX",
      "AVX2",        "AVX512BW", "AVX512CD",      "AVX512DQ",
      "AVX512ER",    "AVX512F",  "AVX512_4FMAPS", "AVX512_4VNNIW",
      "AVX512_IFMA", "AVX512PF", "AVX512_VBMI",   "AVX512VL",
      "BMI1",        "BMI2",     "CLMUL",         "CLWB",
      "F16C",        "FMA",      "FPU",           "FSGSBASE",
      "HLE",         "INVPCID",  "LZCNT",         "MMX",
      "MPX",         "OSPKE",    "PRFCHW",        "RDPID",
      "RDRAND",      "RDSEED",   "RTM",           "SHA",
      "SMAP",        "SSE",      "SSE2",          "SSE3",
      "SSE4_1",      "SSE4_2",   "SSSE3",         "XSAVE",
      "XSAVEC",      "XSS",      "XSAVEOPT"};
  return *kValidFeatures;
}

using OperandEncoding =
    InstructionTable::OperandEncodingCrossref::OperandEncoding;
using OperandEncodingMatchers =
    std::vector<std::pair<OperandEncoding::OperandEncodingSpec, RE2*>>;

const OperandEncodingMatchers& GetOperandEncodingSpecMatchers() {
  // See unit tests for examples.
  static const auto* kOperandEncodingSpec = new OperandEncodingMatchers{{
      {OperandEncoding::OE_NA, new RE2("NA")},
      {OperandEncoding::OE_VEX_SUFFIX, new RE2(R"(imm8\[7:4\])")},
      {OperandEncoding::OE_IMMEDIATE,
       new RE2(
           R"((?:(?:[iI]mm(?:\/?(?:8|16|26|32|64)){1,4})(?:\[[0-9]:[0-9]\])?|Offset|Moffs|iw)(?:\s+\(([wW, rR]+)\))?)")},
      {OperandEncoding::OE_MOD_REG, new RE2(R"(ModRM:reg\s+\(([rR, wW]+)\))")},
      {OperandEncoding::OE_MOD_RM,
       new RE2(
           R"(ModRM:r/?m\s*\(([rR, wW]+)(?:ModRM:\[[0-9]+:[0-9]+\] must (?:not )?be [01]+b)?\))")},
      {OperandEncoding::OE_VEX,
       new RE2(R"(VEX\.(?:[1v]{4})(?:\s+\(([rR, wW]+)\))?)")},
      {OperandEncoding::OE_EVEX_V,
       new RE2(R"((?:EVEX\.)?(?:v{4})(?:\s+\(([rR, wW]+)\))?)")},
      {OperandEncoding::OE_OPCODE,
       new RE2(R"(opcode\s*\+\s*rd\s+\(([rR, wW]+)\))")},
      {OperandEncoding::OE_IMPLICIT,
       new RE2(R"([Ii]mplicit XMM0(?:\s+\(([rR, wW]+)\))?)")},
      {OperandEncoding::OE_REGISTERS,
       new RE2(
           R"(<?[A-Z][A-Z0-9]+>?(?:/<?[A-Z][A-Z0-9]+>?)*(?:\s+\(([rR, wW]+)\))?)")},
      {OperandEncoding::OE_REGISTERS2,
       new RE2(R"(RDX/EDX is implied 64/32 bits \nsource)")},
      {OperandEncoding::OE_CONSTANT, new RE2(R"([0-9])")},
      {OperandEncoding::OE_SIB,
       new RE2(
           R"(SIB\.base\s+\(r\):\s+Address of pointer\nSIB\.index\(r\))")},
      {OperandEncoding::OE_VSIB,
       new RE2(R"(BaseReg \(R\): VSIB:base,\nVectorReg\(R\): VSIB:index)")},
  }};
  return *kOperandEncodingSpec;
}

void Cleanup(std::string* text) {
  absl::StripAsciiWhitespace(text);
  while (!text->empty() && text->back() == '*') text->pop_back();
}

bool IsValidMode(const std::string& text) {
  InstructionTable::Mode mode;
  if (TryParse(GetInstructionModeMatchers(), text, &mode) != nullptr) {
    return mode == InstructionTable::MODE_V;
  }
  return false;
}

// We want to normalize features to the set defined by GetValidFeatureSet or
// logical composition of them (several features separated by '&&' or '||')
// TODO(gchatelet): Move this to configuration file.
std::string FixFeature(std::string feature) {
  absl::StripAsciiWhitespace(&feature);
  RE2::GlobalReplace(&feature, R"([\n-])", "");
  const char kAvxRegex[] =
      "(AVX512BW|AVX512CD|AVX512DQ|AVX512ER|AVX512F|AVX512_IFMA|AVX512PF|"
      "AVX512_VBMI|AVX512VL)+";
  if (RE2::FullMatch(feature, kAvxRegex)) {
    absl::string_view remainder(feature);
    absl::string_view piece;
    std::vector<absl::string_view> pieces;
    while (RE2::Consume(&remainder, kAvxRegex, &piece)) {
      pieces.push_back(piece);
    }
    return absl::StrJoin(pieces, " && ");
  }
  if (feature == "Both AES andAVX flags") return "AES && AVX";
  if (feature == "Both PCLMULQDQ and AVX flags") return "CLMUL && AVX";
  if (feature == "HLE or RTM") return "HLE || RTM";
  if (feature == "PCLMULQDQ") return "CLMUL";
  if (feature == "PREFETCHWT1") return "3DNOW";
  if (feature == "HLE1") return "HLE";
  return feature;
}

// Applies transformations to normalize binary encoding.
// TODO(gchatelet): Move this to document specific configuration.
std::string FixEncodingSpecification(std::string feature) {
  absl::StripAsciiWhitespace(&feature);
  RE2::GlobalReplace(&feature, R"([,\n])", " ");  // remove commas and LF
  RE2::GlobalReplace(&feature, R"([ ]+)", " ");   // collapse multiple spaces

  // remove unnecessary '¹'
  RE2::GlobalReplace(&feature, R"(/r1$)", "/r");
  RE2::GlobalReplace(&feature, R"(ib1$)", "ib");
  RE2::GlobalReplace(&feature, R"(VEX\.NDS1\.LZ)", "VEX.NDS.LZ");

  RE2::GlobalReplace(&feature, R"(\*)", "");  // remove asterisks.

  RE2::GlobalReplace(&feature, R"(REX\.w)", "REX.W");  // wrong case for w
  RE2::GlobalReplace(&feature, R"(A8ib)", "A8 ib");    // missing space
  return feature;
}

const LazyRE2 kInstructionRegexp = {R"(\n([A-Z][0-9A-Z]+))"};

void ParseCell(const InstructionTable::Column column, std::string text,
               InstructionProto* instruction) {
  absl::StripAsciiWhitespace(&text);
  switch (column) {
    case InstructionTable::IT_OPCODE:
      instruction->set_raw_encoding_specification(
          FixEncodingSpecification(text));
      break;
    case InstructionTable::IT_INSTRUCTION:
      ParseVendorSyntax(text, instruction->mutable_vendor_syntax());
      break;
    case InstructionTable::IT_OPCODE_INSTRUCTION: {
      std::string mnemonic;
      if (RE2::PartialMatch(text, *kInstructionRegexp, &mnemonic)) {
        const size_t index_of_mnemonic = text.find(mnemonic);
        CHECK_NE(index_of_mnemonic, absl::string_view::npos);
        const std::string opcode_text = text.substr(0, index_of_mnemonic);
        const std::string instruction_text = text.substr(index_of_mnemonic);
        ParseVendorSyntax(instruction_text,
                          instruction->mutable_vendor_syntax());
        instruction->set_raw_encoding_specification(
            FixEncodingSpecification(opcode_text));
      } else {
        LOG(ERROR) << "Unable to separate opcode from instruction in " << text
                   << ", setting to " << kUnknown;
        instruction->set_raw_encoding_specification(kUnknown);
      }
      break;
    }
    case InstructionTable::IT_DESCRIPTION:
      instruction->set_description(CleanupParagraph(text));
      break;
    case InstructionTable::IT_MODE_COMPAT_LEG:
      instruction->set_legacy_instruction(IsValidMode(text));
      break;
    case InstructionTable::IT_MODE_SUPPORT_64BIT:
      instruction->set_available_in_64_bit(IsValidMode(text));
      break;
    case InstructionTable::IT_MODE_SUPPORT_64_32BIT: {
      const std::vector<std::string> pieces =
          absl::StrSplit(text, "/");  // NOLINT
      instruction->set_available_in_64_bit(IsValidMode(pieces[0]));
      if (pieces.size() == 2) {
        instruction->set_legacy_instruction(IsValidMode(pieces[1]));
      } else {
        LOG(ERROR) << "Invalid 64/32 mode support std::string '" << text << "'";
      }
      break;
    }
    case InstructionTable::IT_OP_EN:
      Cleanup(&text);
      instruction->set_encoding_scheme(text);
      break;
    case InstructionTable::IT_FEATURE_FLAG: {
      // Feature flags are not always consitent. FixFeature makes sure cleaned
      // is one of the valid feature values.
      const std::string cleaned = FixFeature(text);
      std::string* feature_name = instruction->mutable_feature_name();
      for (const absl::string_view piece : absl::StrSplit(cleaned, ' ')) {
        if (!feature_name->empty()) feature_name->append(" ");
        const bool is_logic_operator = piece == "&&" || piece == "||";
        if (is_logic_operator || ContainsKey(GetValidFeatureSet(), piece)) {
          absl::StrAppend(feature_name, piece);
        } else {
          feature_name->append(kUnknown);
          LOG(ERROR) << "Invalid Feature : " << piece
                     << " when parsing : " << cleaned
                     << ", this will be replaced by " << kUnknown;
        }
      }
      break;
    }
    default:
      LOG(ERROR) << "Don't know how to handle cell '" << text << "'";
  }
}

void ParseInstructionTable(const SubSection& sub_section,
                           InstructionTable* table) {
  CHECK(sub_section.rows_size()) << "sub_section must have rows";
  // First we collect the content of the table and get rid of redundant header
  // lines.
  std::vector<PdfTextTableRow> rows;
  for (const auto& row : sub_section.rows()) {
    if (table->columns().empty()) {
      // Columns are empty, we are parsing the header of the instruction table.
      for (const auto& block : row.blocks()) {
        CHECK(!block.text().empty())
            << "empty text block while parsing instruction table header, "
               "current subsection : "
            << sub_section.DebugString();
        InstructionTable::Column column;
        if (TryParse(GetInstructionColumnMatchers(), block.text(), &column) !=
            nullptr) {
          table->add_columns(column);
        } else {
          table->add_columns(InstructionTable::IT_UNKNOWN);
          LOG(ERROR) << "Unable to parse instruction table header "
                     << block.text();
        }
      }
    } else {
      // Header is parsed, we have a set of valid columns and we start to parse
      // a row of the instruction table.
      const std::string& first_cell = row.blocks(0).text();
      // Sometimes there are notes after the instruction table if so we stop the
      // parsing.
      if (absl::StartsWith(first_cell, "NOTE")) {
        break;
      }
      // Checking if this line is a repeated header row.
      const auto first_cell_type =
          ParseWithDefault(GetInstructionColumnMatchers(), first_cell,
                           InstructionTable::IT_UNKNOWN);
      const auto& first_column_type = table->columns(0);
      if (first_cell_type == first_column_type) {
        continue;
      }
      rows.push_back(row);
    }
  }
  const auto& columns = table->columns();
  if (columns.size() <= 3) {
    LOG(ERROR) << "Discarding Instruction Table with less than 4 columns.";
    return;
  }
  // Sometimes for IT_OPCODE_INSTRUCTION columns, the instruction is on a
  // separate line so we want to put it back the previous line.
  if (columns.Get(0) == InstructionTable::IT_OPCODE_INSTRUCTION) {
    PdfTextTableRow* previous = nullptr;
    for (auto& row : rows) {
      if (previous && row.blocks().size() == 1) {
        auto* previous_text = previous->mutable_blocks(0)->mutable_text();
        previous_text->push_back('\n');
        previous_text->append(row.blocks().Get(0).text());
      }
      previous = &row;
    }
    // Removing lonely lines.
    rows.erase(std::remove_if(rows.begin(), rows.end(),
                              [](const PdfTextTableRow& row) {
                                return row.blocks_size() == 1;
                              }),
               rows.end());
  }
  // Parse instructions
  for (const auto& row : rows) {
    if (row.blocks_size() != columns.size()) break;  // end of the table
    auto* instruction = table->add_instructions();
    int i = 0;
    for (const auto& block : row.blocks()) {
      ParseCell(table->columns(i++), block.text(), instruction);
    }
  }
}

OperandEncodingTableType GetOperandEncodingTableHeaderType(
    const PdfTextTableRow& row) {
  bool has_tuple_type_column = false;
  for (const auto& block : row.blocks()) {
    std::string text = block.text();
    RemoveSpaceAndLF(&text);
    if (text == "TupleType") {
      has_tuple_type_column = true;
    }
    if (!RE2::FullMatch(text, R"(Op/En|Operand[1234]|Tuple(Type)?)")) {
      return OET_INVALID;
    }
  }
  return has_tuple_type_column ? OET_WITH_TUPLE_TYPE : OET_LEGACY;
}

void ParseOperandEncodingTableRow(const OperandEncodingTableType table_type,
                                  const PdfTextTableRow& row,
                                  InstructionTable* table) {
  CHECK(table_type == OET_WITH_TUPLE_TYPE || table_type == OET_LEGACY);
  const int first_operand_index = table_type == OET_LEGACY ? 1 : 2;
  // First the operand specs.
  std::vector<OperandEncoding> operand_encodings;
  for (int i = first_operand_index; i < row.blocks_size(); ++i) {
    operand_encodings.push_back(
        ParseOperandEncodingTableCell(row.blocks(i).text()));
  }
  // The cell can specify several cross references (e.g. "HVM, QVM, OVM")
  // We instantiate as many operand encoding as cross references.
  const std::vector<absl::string_view> cross_references =
      absl::StrSplit(row.blocks(0).text(), ',', absl::SkipEmpty());
  for (absl::string_view cross_reference : cross_references) {
    cross_reference = absl::StripAsciiWhitespace(cross_reference);
    if (RE2::FullMatch(cross_reference, R"([A-Z][-A-Z0-9]*)")) {
      auto* const crossref = table->add_operand_encoding_crossrefs();
      crossref->set_crossreference_name(std::string(cross_reference));
      for (const auto& encoding : operand_encodings) {
        *crossref->add_operand_encodings() = encoding;
      }
    } else {
      LOG(ERROR) << "Bypassing invalid cross-reference '" << cross_reference
                 << "'";
    }
  }
}

// Extracts information from the Operand Encoding Table.
// For each row in the table we create an operand_encoding containing a
// crossreference_name and a list of operand_encoding_specs.
void ParseOperandEncodingTable(const SubSection& sub_section,
                               InstructionTable* table) {
  size_t column_count = 0;
  OperandEncodingTableType table_type = OET_INVALID;
  for (const auto& row : sub_section.rows()) {
    if (column_count == 0) {
      // Parsing the operand encoding table header, we just make sure the text
      // is valid but don't store any informations.
      column_count = row.blocks_size();
      table_type = GetOperandEncodingTableHeaderType(row);
      CHECK_NE(table_type, OET_INVALID)
          << "Invalid operand header " << row.DebugString();
    } else {
      // Skipping redundant header.
      if (GetOperandEncodingTableHeaderType(row) == table_type) {
        continue;
      }
      // Stop parsing if we're out of the table.
      if (row.blocks_size() != column_count) {
        break;
      }
      // Parsing a operand encoding table row.
      ParseOperandEncodingTableRow(table_type, row, table);
    }
  }
}

// Read pages and gathers lines that belong to a particular SubSection (e.g.
// "Description", "Operand Encoding Table", "Affected Flags"...)
std::vector<SubSection> ExtractSubSectionRows(const Pages& pages) {
  std::vector<SubSection> output;
  bool first_row = true;
  SubSection current;
  for (const auto* page : pages) {
    for (const auto* pdf_row : GetPageBodyRows(*page, kPageMargin)) {
      const std::string section_title = GetSubSectionTitle(*pdf_row);
      const SubSection::Type section_type =
          first_row ? SubSection::INSTRUCTION_TABLE
                    : ParseWithDefault(GetSubSectionMatchers(), section_title,
                                       SubSection::UNKNOWN);
      if (section_type != SubSection::UNKNOWN) {
        output.push_back(current);
        current.Clear();
        current.set_type(section_type);
      } else {
        PdfTextTableRow row = *pdf_row;
        for (auto& block : *row.mutable_blocks()) {
          block.clear_bounding_box();
          block.clear_font_size();
        }
        row.clear_bounding_box();
        row.Swap(current.add_rows());
      }
      first_row = false;
    }
  }
  output.push_back(current);
  return output;
}

// This function sets the proper encoding for each instruction by looking it up
// in the Operand Encoding Table. Duplicated identifiers in the Operand Encoding
// Table are discarded and encoding is set to ANY_ENCODING.
void PairOperandEncodings(InstructionSection* section) {
  auto* table = section->mutable_instruction_table();
  std::map<std::string, const InstructionTable::OperandEncodingCrossref*>
      mapping;
  std::set<std::string> duplicated_crossreference;
  for (const auto& operand_encoding : table->operand_encoding_crossrefs()) {
    const std::string& cross_reference = operand_encoding.crossreference_name();
    if (!InsertIfNotPresent(&mapping, cross_reference, &operand_encoding)) {
      LOG(ERROR) << "Duplicated Operand Encoding Scheme for " << section->id()
                 << ", this will result in UNKNOWN operand encoding sheme";
      InsertIfNotPresent(&duplicated_crossreference, cross_reference);
    }
  }
  // Removing duplicated reference, they will be encoded as ANY_ENCODING.
  for (const std::string& duplicated : duplicated_crossreference) {
    mapping[duplicated] = nullptr;
  }
  // Assigning encoding specifications to all instructions.
  for (auto& instruction : *table->mutable_instructions()) {
    std::string encoding_scheme = instruction.encoding_scheme();
    RemoveSpaceAndLF(&encoding_scheme);
    if (encoding_scheme.empty()) {
      continue;
    }
    if (!ContainsKey(mapping, encoding_scheme)) {
      LOG(ERROR) << "Unable to find crossreference " << encoding_scheme
                 << " in Operand Encoding Table";
      continue;
    }
    const auto* encoding = FindOrDie(mapping, encoding_scheme);
    auto* vendor_syntax = instruction.mutable_vendor_syntax();
    for (int i = 0; i < vendor_syntax->operands_size(); ++i) {
      auto* operand = vendor_syntax->mutable_operands(i);
      const auto spec = encoding == nullptr
                            ? OperandEncoding::OE_NA
                            : encoding->operand_encodings(i).spec();
      switch (spec) {
        case OperandEncoding::OE_NA:
          // Do not set the encoding if we can't detect it properly from the
          // data in the manual. It will be filled in the cleanup phase by
          // AddOperandInfo() based on what encoding "slots" are provided by the
          // encoding of the instruction, and what slots are used by the other
          // operands.
          operand->clear_encoding();
          break;
        case OperandEncoding::OE_IMMEDIATE:
          operand->set_encoding(InstructionOperand::IMMEDIATE_VALUE_ENCODING);
          break;
        case OperandEncoding::OE_OPCODE:
          operand->set_encoding(InstructionOperand::OPCODE_ENCODING);
          break;
        case OperandEncoding::OE_SIB:
        case OperandEncoding::OE_MOD_RM:
          operand->set_encoding(InstructionOperand::MODRM_RM_ENCODING);
          break;
        case OperandEncoding::OE_MOD_REG:
          operand->set_encoding(InstructionOperand::MODRM_REG_ENCODING);
          break;
        case OperandEncoding::OE_IMPLICIT:
        case OperandEncoding::OE_REGISTERS:
        case OperandEncoding::OE_REGISTERS2:
        case OperandEncoding::OE_CONSTANT:
          operand->set_encoding(InstructionOperand::IMPLICIT_ENCODING);
          break;
        case OperandEncoding::OE_VEX:
        case OperandEncoding::OE_EVEX_V:
          operand->set_encoding(InstructionOperand::VEX_V_ENCODING);
          break;
        case OperandEncoding::OE_VSIB:
          operand->set_encoding(InstructionOperand::VSIB_ENCODING);
          break;
        case OperandEncoding::OE_VEX_SUFFIX:
          operand->set_encoding(InstructionOperand::VEX_SUFFIX_ENCODING);
          break;
        default:
          LOG(FATAL) << "Don't know how to handle "
                     << OperandEncoding::OperandEncodingSpec_Name(spec);
      }
      const auto usage = encoding == nullptr
                             ? OperandEncoding::USAGE_UNKNOWN
                             : encoding->operand_encodings(i).usage();
      switch (usage) {
        case OperandEncoding::USAGE_UNKNOWN:
          break;
        case OperandEncoding::USAGE_READ:
          operand->set_usage(InstructionOperand::USAGE_READ);
          break;
        case OperandEncoding::USAGE_WRITE:
          operand->set_usage(InstructionOperand::USAGE_WRITE);
          break;
        case OperandEncoding::USAGE_READ_WRITE:
          operand->set_usage(InstructionOperand::USAGE_READ_WRITE);
          break;
        default:
          LOG(FATAL) << "Don't know how to handle "
                     << OperandEncoding::Usage_Name(usage);
      }
    }
  }
}

// Process the sub sections of the instructions and extract relevant data.
void ProcessSubSections(std::vector<SubSection> sub_sections,
                        InstructionSection* section) {
  for (SubSection& sub_section : sub_sections) {
    // Discard empty sections.
    if (sub_section.rows().empty()) {
      continue;
    }
    // Process
    auto* instruction_table = section->mutable_instruction_table();
    switch (sub_section.type()) {
      case SubSection::INSTRUCTION_TABLE:
        ParseInstructionTable(sub_section, instruction_table);
        break;
      case SubSection::INSTRUCTION_OPERAND_ENCODING:
        ParseOperandEncodingTable(sub_section, instruction_table);
        break;
      default:
        break;
    }
    sub_section.Swap(section->add_sub_sections());
  }
  PairOperandEncodings(section);
}

// Outputs a row to a string separating cells by tabulations.
// TODO(gchatelet): if one of the block's text contains a tab or a line feed the
// resulting formatting will be broken. Nevertheless after looking at a few
// examples, tab separated cells seems to be a good strategy.
void ToString(const PdfTextTableRow& row, std::string* output) {
  bool first_block = true;
  for (const auto& block : row.blocks()) {
    if (!first_block) absl::StrAppend(output, "\t");
    absl::StrAppend(output, CleanupParagraph(block.text()));
    first_block = false;
  }
}

// Outputs a section to a string separating rows by line feeds.
// If type is not filled returns false.
bool ToString(const InstructionSection& section, const SubSection::Type type,
              std::string* output) {
  CHECK(output);
  const auto& sub_sections = section.sub_sections();
  const auto itr = std::find_if(sub_sections.begin(), sub_sections.end(),
                                [type](const SubSection& sub_section) {
                                  return sub_section.type() == type;
                                });
  if (itr == sub_sections.end()) {
    return false;
  }
  output->clear();
  for (const auto& row : itr->rows()) {
    if (!output->empty()) absl::StrAppend(output, "\n");
    ToString(row, output);
  }
  return true;
}

// Fills InstructionGroupProto with subsections.
void FillGroupProto(const InstructionSection& section,
                    InstructionGroupProto* group) {
  const size_t first_hyphen_position = section.id().find('-');
  if (first_hyphen_position == std::string::npos) {
    group->set_name(section.id());
  } else {
    std::string name = section.id().substr(0, first_hyphen_position);
    std::string description = section.id().substr(first_hyphen_position + 1);
    absl::StripAsciiWhitespace(&name);
    absl::StripAsciiWhitespace(&description);
    group->set_name(name);
    group->set_short_description(description);
  }
  std::string buffer;
  if (ToString(section, SubSection::DESCRIPTION, &buffer)) {
    group->set_description(buffer);
  }
  for (const auto type :
       {SubSection::FLAGS_AFFECTED, SubSection::FLAGS_AFFECTED_FPU,
        SubSection::FLAGS_AFFECTED_INTEGER}) {
    if (ToString(section, type, &buffer)) {
      group->add_flags_affected()->set_content(buffer);
    }
  }
}

}  // namespace

OperandEncoding ParseOperandEncodingTableCell(const std::string& content) {
  OperandEncoding::OperandEncodingSpec spec = OperandEncoding::OE_NA;
  const RE2* const regexp =
      content.empty()
          ? nullptr
          : TryParse(GetOperandEncodingSpecMatchers(), content, &spec);
  if (regexp == nullptr) {
    LOG(INFO) << "Cannot match '" << content << "', falling back to default";
  }
  OperandEncoding encoding;
  encoding.set_spec(spec);
  switch (spec) {
    case OperandEncoding::OE_NA:
      break;
    case OperandEncoding::OE_IMMEDIATE:
    case OperandEncoding::OE_CONSTANT:
    case OperandEncoding::OE_SIB:
    case OperandEncoding::OE_VSIB:
      encoding.set_usage(OperandEncoding::USAGE_READ);
      break;
    case OperandEncoding::OE_MOD_RM:
    case OperandEncoding::OE_MOD_REG:
    case OperandEncoding::OE_OPCODE:
    case OperandEncoding::OE_VEX:
    case OperandEncoding::OE_EVEX_V:
    case OperandEncoding::OE_IMPLICIT:
    case OperandEncoding::OE_REGISTERS:
    case OperandEncoding::OE_REGISTERS2: {
      CHECK(regexp != nullptr);
      std::string usage;
      if (RE2::FullMatch(content, *regexp, &usage) && !usage.empty()) {
        absl::AsciiStrToLower(&usage);
        RemoveAllChars(&usage, " ,");
        if (usage == "r") {
          encoding.set_usage(OperandEncoding::USAGE_READ);
        } else if (usage == "w") {
          encoding.set_usage(OperandEncoding::USAGE_WRITE);
        } else if (usage == "rw") {
          encoding.set_usage(OperandEncoding::USAGE_READ_WRITE);
        } else {
          LOG(ERROR) << "Unknown usage '" << usage << "' for '" << content
                     << "'";
        }
      } else {
        LOG(ERROR) << "Missing usage for '" << content << "'";
      }
    } break;
    default:
      break;
  }
  return encoding;
}

SdmDocument ConvertPdfDocumentToSdmDocument(
    const exegesis::pdf::PdfDocument& pdf) {
  // Find all instruction pages.
  SdmDocument sdm_document;
  std::map<std::string, Pages> instruction_group_id_to_pages;
  for (int i = 0; i < pdf.pages_size(); ++i) {
    const std::string instruction_group_id =
        GetInstructionGroupId(pdf.pages(i));
    if (instruction_group_id.empty()) continue;
    instruction_group_id_to_pages[instruction_group_id] =
        GetInstructionsPages(pdf, i, instruction_group_id);
  }
  // Now processing instruction pages
  for (const auto& id_pages_pair : instruction_group_id_to_pages) {
    InstructionSection section;
    const auto& group_id = id_pages_pair.first;
    const auto& pages = id_pages_pair.second;
    LOG(INFO) << "Processing section id " << group_id << " pages "
              << pages.front()->number() << "-" << pages.back()->number();
    section.set_id(group_id);
    ProcessSubSections(ExtractSubSectionRows(pages), &section);
    if (section.instruction_table().instructions_size() == 0) {
      LOG(WARNING) << "Empty instruction table, skipping the section";
      continue;
    }
    section.Swap(sdm_document.add_instruction_sections());
  }
  return sdm_document;
}

InstructionSetProto ProcessIntelSdmDocument(const SdmDocument& sdm_document) {
  InstructionSetProto instruction_set;
  for (const auto& section : sdm_document.instruction_sections()) {
    const size_t group_index = instruction_set.instruction_groups_size();
    FillGroupProto(section, instruction_set.add_instruction_groups());
    for (const auto& instruction : section.instruction_table().instructions()) {
      InstructionProto* const new_instruction =
          instruction_set.add_instructions();
      *new_instruction = instruction;
      new_instruction->set_instruction_group_index(group_index);
    }
  }
  return instruction_set;
}

}  // namespace pdf
}  // namespace x86
}  // namespace exegesis
