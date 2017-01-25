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

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cpu_instructions/x86/pdf/pdf_document_utils.h"
#include "cpu_instructions/x86/pdf/vendor_syntax.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "strings/case.h"
#include "strings/str_cat.h"
#include "strings/str_join.h"
#include "strings/str_split.h"
#include "strings/string_view_utils.h"
#include "strings/strip.h"
#include "strings/util.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"

namespace cpu_instructions {
namespace x86 {
namespace pdf {

namespace {

using re2::StringPiece;

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
const RE2* TryParse(const Container& matchers, const string& text,
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
ValueType ParseWithDefault(const Container& matchers, const string& text,
                           const ValueType& default_value) {
  for (const auto& pair : matchers) {
    if (RE2::FullMatch(text, *pair.second)) return pair.first;
  }
  return default_value;
}

typedef std::vector<const PdfPage*> Pages;
typedef std::vector<const PdfTextTableRow*> Rows;
typedef google::protobuf::RepeatedField<InstructionTable::Column> Columns;

void RemoveSpaceAndLF(string* text) { strrmm(text, "\n "); }

constexpr const size_t kMaxInstructionIdSize = 60;
constexpr const char kInstructionSetRef[] = "INSTRUCTION SET REFERENCE";

// This function is used to create a stable id from instruction name found:
// - at the top of a page describing a new instruction
// - in the footer of a page for a particular instruction
// It does so by removing some characters and imposing a limit on the text size.
// Limiting the size is necessary because when text is too long it gets
// truncated in different ways.
string Normalize(string text) {
  strrmm(&text, "\n ∗*");
  if (text.size() > kMaxInstructionIdSize) text.resize(kMaxInstructionIdSize);
  return text;
}

// If page number is even, returns the rightmost string in the footer, else the
// leftmost string.
const string& GetFooterSectionName(const PdfPage& page) {
  return page.number() % 2 == 0 ? GetCellTextOrEmpty(page, -1, -1)
                                : GetCellTextOrEmpty(page, -1, 0);
}

// If 'page' is the first page of an instruction, returns a unique identifier
// for this instruction. Otherwise return empty string.
string GetInstructionGroupId(const PdfPage& page) {
  if (!strings::StartsWith(GetCellTextOrEmpty(page, 0, 0), kInstructionSetRef))
    return {};
  const string maybe_instruction = Normalize(GetCellTextOrEmpty(page, 1, 0));
  const string& footer_section_name = GetFooterSectionName(page);
  if (maybe_instruction == Normalize(footer_section_name)) {
    return footer_section_name;
  }
  return {};
}

// True if page footer's corresponds to the same instruction_id.
bool IsPageInstruction(const PdfPage& page,
                       const string& instruction_group_id) {
  return Normalize(GetFooterSectionName(page)) ==
         Normalize(instruction_group_id);
}

// Returns the list of pages an instruction spans.
std::vector<const PdfPage*> GetInstructionsPages(
    const PdfDocument& document, const int first_page,
    const string& instruction_group_id) {
  std::vector<const PdfPage*> result;
  for (int i = first_page; i < document.pages_size(); ++i) {
    const auto& page = document.pages(i);
    if (!IsPageInstruction(page, instruction_group_id)) break;
    result.push_back(&page);
  }
  return result;
}

constexpr const float kMinSubSectionTitleFontSize = 9.5f;

string GetSubSectionTitle(const PdfTextTableRow& row) {
  if (row.blocks().empty() || row.blocks_size() > 2) return {};
  const auto& block = row.blocks(0);
  if (block.font_size() < kMinSubSectionTitleFontSize) return {};
  string text = block.text();
  StripWhitespace(&text);
  if (strings::StartsWith(text, "Table") ||
      strings::StartsWith(text, "Figure") ||
      strings::StartsWith(text, "Example"))
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

const std::map<InstructionTable::Column, const RE2*>&
GetInstructionColumnMatchers() {
  static const auto* kInstructionColumns =
      new std::map<InstructionTable::Column, const RE2*>{
          {InstructionTable::IT_OPCODE, new RE2(R"(Opcode\*{0,3})")},
          {InstructionTable::IT_OPCODE_INSTRUCTION,
           new RE2(R"(Opcode\*?/?\n?Instruction)")},
          {InstructionTable::IT_INSTRUCTION, new RE2(R"(Instruction)")},
          {InstructionTable::IT_MODE_SUPPORT_64_32BIT,
           new RE2(R"(64/3\n?2\n?[- ]?\n?bit \n?Mode( \n?Support)?)")},
          {InstructionTable::IT_MODE_SUPPORT_64BIT,
           new RE2(R"(64-[Bb]it \n?Mode)")},
          {InstructionTable::IT_MODE_COMPAT_LEG,
           new RE2(R"(Compat/\n?Leg Mode\*?)")},
          {InstructionTable::IT_FEATURE_FLAG,
           new RE2(R"(CPUID(\ ?\n?Fea\-?\n?ture \n?Flag)?)")},
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

const std::set<string>& GetValidFeatureSet() {
  static const auto* kValidFeatures = new std::set<string>{
      "3DNOW",      "ADX",      "AES",        "AVX",      "AVX2",
      "AVX512BW",   "AVX512CD", "AVX512DQ",   "AVX512ER", "AVX512F",
      "AVX512IFMA", "AVX512PF", "AVX512VBMI", "AVX512VL", "BMI1",
      "BMI2",       "CLMUL",    "CLWB",       "F16C",     "FMA",
      "FPU",        "FSGSBASE", "HLE",        "INVPCID",  "LZCNT",
      "MMX",        "MPX",      "OSPKE",      "PRFCHW",   "RDPID",
      "RDRAND",     "RDSEED",   "RTM",        "SHA",      "SMAP",
      "SSE",        "SSE2",     "SSE3",       "SSE4_1",   "SSE4_2",
      "SSSE3",      "XSAVEOPT"};
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
           R"(ModRM:r/?m\s+\(([rR, wW]+)(?:ModRM:\[[0-9]+:[0-9]+\] must (?:not )?be [01]+b)?\))")},
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

void Cleanup(string* text) {
  StripWhitespace(text);
  while (!text->empty() && text->back() == '*') text->pop_back();
}

bool IsValidMode(const string& text) {
  InstructionTable::Mode mode;
  if (TryParse(GetInstructionModeMatchers(), text, &mode) != nullptr) {
    return mode == InstructionTable::MODE_V;
  }
  return false;
}

string CleanupDescription(string input) {
  Cleanup(&input);
  string output;
  for (const char c : input) {
    if (c == '\n') {
      if (!output.empty() && output.back() == '-') {
        output.back() = ' ';
      }
    } else {
      output += c;
    }
  }
  return output;
}

// We want to normalize features to the set defined by GetValidFeatureSet or
// logical composition of them (several features separated by '&&' or '||')
// TODO(user): Move this to configuration file.
string FixFeature(string feature) {
  StripWhitespace(&feature);
  RE2::GlobalReplace(&feature, R"([\n-])", "");
  const char kAvxRegex[] =
      "(AVX512BW|AVX512CD|AVX512DQ|AVX512ER|AVX512F|AVX512IFMA|AVX512PF|"
      "AVX512VBMI|AVX512VL)+";
  if (RE2::FullMatch(feature, kAvxRegex)) {
    StringPiece remainder(feature);
    string piece;
    std::vector<string> pieces;
    while (RE2::Consume(&remainder, kAvxRegex, &piece)) {
      pieces.push_back(piece);
    }
    return strings::Join(pieces, " && ");
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
// TODO(user): Move this to document specific configuration.
string FixEncodingSpecification(string feature) {
  StripWhitespace(&feature);
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

void ParseCell(const InstructionTable::Column column, string text,
               InstructionProto* instruction) {
  StripWhitespace(&text);
  switch (column) {
    case InstructionTable::IT_OPCODE:
      instruction->set_raw_encoding_specification(
          FixEncodingSpecification(text));
      break;
    case InstructionTable::IT_INSTRUCTION:
      ParseVendorSyntax(text, instruction->mutable_vendor_syntax());
      break;
    case InstructionTable::IT_OPCODE_INSTRUCTION: {
      string mnemonic;
      if (RE2::PartialMatch(text, *kInstructionRegexp, &mnemonic)) {
        const size_t index_of_mnemonic = text.find(mnemonic);
        CHECK_NE(index_of_mnemonic, StringPiece::npos);
        const string opcode_text = text.substr(0, index_of_mnemonic);
        const string instruction_text = text.substr(index_of_mnemonic);
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
      // 'text' is multiline, CleanupDescription joins lines and erase
      // hyphenations and removes trailing asterix.
      instruction->set_description(CleanupDescription(text));
      break;
    case InstructionTable::IT_MODE_COMPAT_LEG:
      instruction->set_legacy_instruction(IsValidMode(text));
      break;
    case InstructionTable::IT_MODE_SUPPORT_64BIT:
      instruction->set_available_in_64_bit(IsValidMode(text));
      break;
    case InstructionTable::IT_MODE_SUPPORT_64_32BIT: {
      const std::vector<string> pieces = strings::Split(text, "/");  // NOLINT
      instruction->set_available_in_64_bit(IsValidMode(pieces[0]));
      if (pieces.size() == 2) {
        instruction->set_legacy_instruction(IsValidMode(pieces[1]));
      } else {
        LOG(ERROR) << "Invalid 64/32 mode support string '" << text << "'";
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
      const string cleaned = FixFeature(text);
      string* feature_name = instruction->mutable_feature_name();
      for (StringPiece raw_piece : strings::Split(cleaned, " ")) {  // NOLINT
        const string piece = raw_piece.ToString();
        if (!feature_name->empty()) feature_name->append(" ");
        const bool is_logic_operator = piece == "&&" || piece == "||";
        if (is_logic_operator || ContainsKey(GetValidFeatureSet(), piece)) {
          StrAppend(feature_name, piece);
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
      const string& first_cell = row.blocks(0).text();
      // Sometimes there are notes after the instruction table if so we stop the
      // parsing.
      if (strings::StartsWith(first_cell, "NOTE")) {
        break;
      }
      // Checking if this line is a repeated header row,
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

bool IsOperandEncodingTableHeader(const PdfTextTableRow& row) {
  const auto& blocks = row.blocks();
  return std::all_of(blocks.begin(), blocks.end(),
                     [](const PdfTextBlock& block) {
                       string text = block.text();
                       RemoveSpaceAndLF(&text);
                       return RE2::FullMatch(text,
                                             R"(Op/En|Operand[1234])");
                     });
}

void ParseOperandEncodingTableRow(const PdfTextTableRow& row,
                                  InstructionTable* table) {
  // First the operand specs.
  std::vector<OperandEncoding> operand_encodings;
  for (int i = 1; i < row.blocks_size(); ++i) {
    operand_encodings.push_back(
        ParseOperandEncodingTableCell(row.blocks(i).text()));
  }
  // The cell can specify several cross references (e.g. "HVM, QVM, OVM")
  // We instanciate as many operand encoding as cross references.
  const string& cross_references = row.blocks(0).text();
  for (auto cross_reference :
       strings::Split(cross_references, ",", strings::SkipEmpty())) {  // NOLINT
    StripWhitespace(&cross_reference);
    if (RE2::FullMatch(cross_reference, R"([A-Z][-A-Z0-9]*)")) {
      auto* const crossref = table->add_operand_encoding_crossrefs();
      crossref->set_crossreference_name(cross_reference);
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
  for (const auto& row : sub_section.rows()) {
    if (column_count == 0) {
      // Parsing the operand encoding table header, we just make sure the text
      // is valid but don't store any informations.
      column_count = row.blocks_size();
      CHECK(IsOperandEncodingTableHeader(row)) << "Invalid operand header ";
    } else {
      // Skipping redundant header.
      if (IsOperandEncodingTableHeader(row)) {
        continue;
      }
      // Stop parsing if we're out of the table.
      if (row.blocks_size() != column_count) {
        break;
      }
      // Parsing a operand encoding table row.
      ParseOperandEncodingTableRow(row, table);
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
      const string section_title = GetSubSectionTitle(*pdf_row);
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
  std::map<string, const InstructionTable::OperandEncodingCrossref*> mapping;
  std::set<string> duplicated_crossreference;
  for (const auto& operand_encoding : table->operand_encoding_crossrefs()) {
    const string& cross_reference = operand_encoding.crossreference_name();
    if (!InsertIfNotPresent(&mapping, cross_reference, &operand_encoding)) {
      LOG(ERROR) << "Duplicated Operand Encoding Scheme for " << section->id()
                 << ", this will result in UNKNOWN operand encoding sheme";
      InsertIfNotPresent(&duplicated_crossreference, cross_reference);
    }
  }
  // Removing duplicated reference, they will be encoded as ANY_ENCODING.
  for (const string& duplicated : duplicated_crossreference) {
    mapping[duplicated] = nullptr;
  }
  // Assigning encoding specifications to all instructions.
  for (auto& instruction : *table->mutable_instructions()) {
    string encoding_scheme = instruction.encoding_scheme();
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

}  // namespace

OperandEncoding ParseOperandEncodingTableCell(const string& content) {
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
      string usage;
      if (RE2::FullMatch(content, *regexp, &usage) && !usage.empty()) {
        LowerString(&usage);
        strrmm(&usage, " ,");
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

SdmDocument ConvertPdfDocumentToSdmDocument(const PdfDocument& pdf) {
  // Find all instruction pages.
  SdmDocument sdm_document;
  std::map<string, Pages> instruction_group_id_to_pages;
  for (int i = 0; i < pdf.pages_size(); ++i) {
    const string instruction_group_id = GetInstructionGroupId(pdf.pages(i));
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
    section.Swap(sdm_document.add_instruction_sections());
  }
  return sdm_document;
}

InstructionSetProto ProcessIntelSdmDocument(const SdmDocument& sdm_document) {
  InstructionSetProto instruction_set;
  for (const auto& section : sdm_document.instruction_sections()) {
    for (const auto& instruction : section.instruction_table().instructions()) {
      InstructionProto* const new_instruction =
          instruction_set.add_instructions();
      *new_instruction = instruction;
      new_instruction->set_group_id(section.id());
    }
  }
  return instruction_set;
}

}  // namespace pdf
}  // namespace x86
}  // namespace cpu_instructions
