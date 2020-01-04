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

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/pdf/pdf_document_utils.h"
#include "exegesis/util/strings.h"
#include "exegesis/util/text_processing.h"
#include "exegesis/x86/pdf/vendor_syntax.h"
#include "glog/logging.h"
#include "net/proto2/util/public/repeated_field_util.h"
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

// We categorise instructions into four classes:
//  VMX: VMX instructions.
//  SGX: Encalve instructions, including ENCLS, ENCLU and ENCLV.
//  LEAF_SGX: Leaf functions available with ENCLS, ENCLU or ENCLV.
//  REGULAR: Other instructions in the manual.
enum InstructionType { REGULAR = 0, VMX = 1, SGX = 2, LEAF_SGX = 3 };
constexpr absl::string_view kInstructionTypeText[] = {"REGULAR", "VMX", "SGX",
                                                      "LEAF_SGX"};
absl::string_view ToString(const InstructionType& instruction_type) {
  return kInstructionTypeText[instruction_type];
}

// Names of SGX instructions. Note that the order of these strings must match
// the order of elements in ParseContext::sgx_instructions_set.
constexpr absl::string_view kSgxInstructionMnemonics[] = {"ENCLS", "ENCLU",
                                                          "ENCLV"};

// Number of SGX main instructions.
constexpr int kSgxInstructionsCount = 3;

int GetSgxIndexByName(absl::string_view mnemonic) {
  for (int i = 0; i < kSgxInstructionsCount; ++i) {
    if (kSgxInstructionMnemonics[i] == mnemonic) return i;
  }
  LOG(FATAL) << "Unknown mnemonic " << mnemonic;
  return -1;
}

// Class to support statshing additional information as we parse the pages.
//
// Some fields which correlate to the current InstructionProto are resettable
// for each InstructionProto that we parse, and some fields are kept around to
// the end of the parse phase.
class ParseContext {
 public:
  void Reset() {
    section_index_ = 0;
    instruction_index_ = 0;
    main_sgx_index_ = -1;
    registers_.clear();
    instruction_type_ = InstructionType::REGULAR;
  }

  // ---------- Getters and Setters ---------------
  const InstructionType& instruction_type() const { return instruction_type_; }
  void set_instruction_type(InstructionType type) { instruction_type_ = type; }

  int section_index() const { return section_index_; }
  void set_section_index(int index) { section_index_ = index; }

  int instruction_index() const { return instruction_index_; }
  void set_instruction_index(int index) { instruction_index_ = index; }

  int GetRegistersCount() { return registers_.size(); }
  absl::string_view GetRegister(int index) const { return registers_[index]; }
  void AddRegister(std::string register_name) {
    registers_.push_back(std::move(register_name));
  }

  // Registers the main SGX instruction. This will fail if we have already
  // registered an instruction with this same mnemonic but which is different
  // than this instruction.
  void AddMainSgxInstruction(absl::string_view main_mnemonic,
                             InstructionProto* instruction);

  // Adds the InstructionProto to the set of leaf instructions.
  // main_mnemonic: the mnemonic of the SGX main instruction that this leaf
  //                belongs to.
  // leaf: the pointer to the leaf instruction in the SdmDocument.
  void AddLeafSgxInstruction(absl::string_view main_mnemonic,
                             InstructionProto* leaf);

  // Adds a the given register and its concreate value as a new
  // InstructionOperand to the given leaf_sgx instruction.
  void AddRegisterOperandValue(InstructionProto* leaf_sgx,
                               std::string register_name,
                               absl::string_view value);

  // Adds a the given register and description as a new
  // InstructionOperand to the given leaf_sgx instruction.
  void AddRegisterOperandDescription(InstructionProto* leaf_sgx,
                                     absl::string_view register_name,
                                     absl::string_view description);

  // ---------- Helper methods ---------------
  // Returns true IFF the current instruction is a leaf SGX function.
  bool IsLeafSgx() const {
    return instruction_type_ == InstructionType::LEAF_SGX;
  }

  // Returns true IFF the current instruction is a VMX intruction.
  bool IsVmx() const { return instruction_type_ == InstructionType::VMX; }

  // Returns the string reperensentation of the current instruction's type.
  absl::string_view GetInstructionTypeAsString() const {
    return ToString(instruction_type_);
  }

  // Adds the leaf SGX instructions into their main InstructionProto and removes
  // them from the SdmDocument.
  void RelocateSgxLeafInstructions(SdmDocument* sdm_document);

 private:
  // Returns the implicit-register Encoding corresponding to the given name.
  static InstructionOperand::Encoding GetRegisterEncodingByName(
      absl::string_view name);

  // Adds a new register operand for the given leaf_sgx and returns the mutable
  // InstructionOperand.
  InstructionOperand* AddRegisterOperand(InstructionProto* leaf_sgx,
                                         std::string register_name);

  // ---------- Resettable fields ---------------
  // Type of the current instruction.
  InstructionType instruction_type_;
  // Current section index.
  int section_index_;
  // Current instruction index.
  int instruction_index_;
  // The index corresponding to the main SGX instruction. Only applicable if
  // the current instruction is a leaf SGX instruction.
  int main_sgx_index_ = -1;
  // Vector of registers in the operand-encoding table. Only applicable if the
  // current instruction is a leaf SGX instruction.
  std::vector<std::string> registers_;

  // ------------ Persistent fields -----------

  // Represents an SGX instruction (ENCLU, ENCLS or ENCLUV) and its leaf
  // instructions.
  struct SgxInstructionsSet {
    // Pointer to InstructionProto of the main instruction in the SdmDocument.
    InstructionProto* main_instruction = nullptr;

    // Set of pointers to SGX leaf-instructions in the SdmDocument.
    absl::flat_hash_set<InstructionProto*> leaf_instructions;

    // Set of section_index of the InstructionProto in leaf_instructions.
    //
    // We use these to quickly locate them in the SdmDocument and remove them
    // after they have been moved into their main InstructionProto's.
    absl::flat_hash_set<int> section_indices;
  };
  // Array of SgxInstructionsSet objects, each of which corresponds to a set of
  // SGX-leaf instructions with a common main instruction.
  SgxInstructionsSet sgx_instructions_set_[kSgxInstructionsCount];
};

// ------------- Start ParseContext definition----------------------------------
void ParseContext::AddMainSgxInstruction(absl::string_view main_mnemonic,
                                         InstructionProto* instruction) {
  CHECK(instruction != nullptr);
  const int index = GetSgxIndexByName(main_mnemonic);
  if (sgx_instructions_set_[index].main_instruction != nullptr) {
    CHECK_EQ(sgx_instructions_set_[index].main_instruction, instruction)
        << "InstructionProto pointer was set to a different value for "
        << main_mnemonic << ": "
        << sgx_instructions_set_[index].main_instruction->DebugString()
        << " VS " << instruction->DebugString();
  } else {
    sgx_instructions_set_[index].main_instruction = instruction;
  }
}

// Adds the InstructionProto to the set of leaf instructions.
// main_mnemonic: the mnemonic of the SGX main instruction that this leaf
//                belongs to.
// leaf: the pointer to the leaf instruction in the SdmDocument.
void ParseContext::AddLeafSgxInstruction(absl::string_view main_mnemonic,
                                         InstructionProto* leaf) {
  CHECK(leaf != nullptr);
  const int main_sgx_index = GetSgxIndexByName(main_mnemonic);
  CHECK(main_sgx_index_ == -1 || main_sgx_index_ == main_sgx_index)
      << "Inconsistent state. Seeing a different main-instruction index: "
      << main_sgx_index_ << " vs " << main_sgx_index;
  main_sgx_index_ = main_sgx_index;
  sgx_instructions_set_[main_sgx_index_].leaf_instructions.insert(leaf);
  sgx_instructions_set_[main_sgx_index_].section_indices.insert(section_index_);
}

void ParseContext::AddRegisterOperandValue(InstructionProto* leaf_sgx,
                                           std::string register_name,
                                           absl::string_view value) {
  CHECK(main_sgx_index_ >= 0) << "Unknown main SGX instruction.";
  AddRegisterOperand(leaf_sgx, std::move(register_name))
      ->set_value(absl::HexStringToBytes(value));
}

void ParseContext::AddRegisterOperandDescription(
    InstructionProto* leaf_sgx, absl::string_view register_name,
    absl::string_view description) {
  CHECK(main_sgx_index_ >= 0) << "Unknown main SGX instruction.";
  CHECK(leaf_sgx != nullptr);

  // We must've added an operand for EAX already.
  CHECK_EQ(1, leaf_sgx->vendor_syntax_size()) << leaf_sgx->DebugString();
  CHECK(leaf_sgx->vendor_syntax(0).operands_size() >= 1)
      << leaf_sgx->DebugString();
  CHECK_EQ("EAX", leaf_sgx->vendor_syntax(0).operands(0).name())
      << leaf_sgx->DebugString();

  VLOG(1) << "Adding description to " << leaf_sgx->vendor_syntax(0).mnemonic()
          << ":\n"
          << "register_name = " << register_name
          << "\ndescription = " << description;

  if ("EAX" == register_name) {
    *leaf_sgx->mutable_vendor_syntax(0)
         ->mutable_operands(0)
         ->mutable_description() = std::string(description);
  } else {
    *AddRegisterOperand(leaf_sgx, std::string(register_name))
         ->mutable_description() = std::string(description);
  }
}

void ParseContext::RelocateSgxLeafInstructions(SdmDocument* sdm_document) {
  DLOG(INFO) << "*** Relocating leaf instructions.";
  CHECK(sdm_document != nullptr);
  for (auto& sgx_instructions : sgx_instructions_set_) {
    if (sgx_instructions.main_instruction == nullptr) {
      continue;
    }

    // Move all the leaves into the main InstructionProto.
    for (auto& leaf : sgx_instructions.leaf_instructions) {
      // TODO(user): Not sure if std::move() on a proto's field is valid???
      // If it is, should move here rather than copying.
      *sgx_instructions.main_instruction->add_leaf_instructions() =
          std::move(*leaf);
    }

    // Remove the leaf instructions as stand-alone instructions from the
    // SdmDocument.
    for (const auto section_index : sgx_instructions.section_indices) {
      auto* const instructions =
          sdm_document->mutable_instruction_sections(section_index)
              ->mutable_instruction_table()
              ->mutable_instructions();
      instructions->erase(
          std::remove_if(instructions->begin(), instructions->end(),
                         [&sgx_instructions](const InstructionProto& t) {
                           return gtl::ContainsKey(
                               sgx_instructions.leaf_instructions, &t);
                         }),
          instructions->end());
    }
  }
}

InstructionOperand::Encoding ParseContext::GetRegisterEncodingByName(
    absl::string_view name) {
  static const absl::flat_hash_map<std::string, InstructionOperand::Encoding>
      encodings = {{"EAX", InstructionOperand_Encoding_X86_REGISTER_EAX},
                   {"EBX", InstructionOperand_Encoding_X86_REGISTER_EBX},
                   {"RAX", InstructionOperand_Encoding_X86_REGISTER_RAX},
                   {"RBX", InstructionOperand_Encoding_X86_REGISTER_RBX},
                   {"RCX", InstructionOperand_Encoding_X86_REGISTER_RCX},
                   {"RDX", InstructionOperand_Encoding_X86_REGISTER_RDX}};

  return gtl::FindOrDie(encodings, std::string(name));
}

InstructionOperand* ParseContext::AddRegisterOperand(
    InstructionProto* leaf_sgx, std::string register_name) {
  CHECK(!register_name.empty());
  VLOG(1) << "Adding register operand " << register_name;
  auto* const vendor_syntax = leaf_sgx->vendor_syntax_size() == 0
                                  ? leaf_sgx->add_vendor_syntax()
                                  : leaf_sgx->mutable_vendor_syntax(0);
  if (vendor_syntax->mnemonic().empty()) {
    *vendor_syntax->mutable_mnemonic() = leaf_sgx->llvm_mnemonic();
  } else {
    CHECK_EQ(vendor_syntax->mnemonic(), leaf_sgx->llvm_mnemonic());
  }

  auto* const operand = vendor_syntax->add_operands();
  operand->mutable_data_type()->set_kind(
      InstructionOperand_DataType_Kind_INTEGER);
  operand->mutable_data_type()->set_bit_width(64);
  operand->set_encoding(GetRegisterEncodingByName(register_name));
  *operand->mutable_name() = std::move(register_name);
  return operand;
}

// ------------- End ParseContext definition------------------------------------

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

constexpr const char kInstructionSetRef[] = "INSTRUCTION SET REFERENCE";
constexpr const char kVmxInstructionRef[] = "VMX INSTRUCTION REFERENCE";
constexpr const char kSgxInstructionRef[] = "SGX INSTRUCTION REFERENCE";

// If page number is even, returns the rightmost string in the footer, else the
// leftmost string.
const std::string& GetFooterSectionName(const PdfPage& page) {
  return page.number() % 2 == 0 ? GetCellTextOrEmpty(page, -1, -1)
                                : GetCellTextOrEmpty(page, -1, 0);
}

// True if the lhs and rhs are the same instruction
bool SameInstructionName(const std::string& lhs, const std::string& rhs) {
  return NormalizeName(lhs) == NormalizeName(rhs);
}

// True if page footer's corresponds to the same instruction_id.
bool IsPageInstruction(const PdfPage& page,
                       const std::string& instruction_group_id) {
  return SameInstructionName(GetFooterSectionName(page), instruction_group_id);
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
  static const auto* kValidFeatures =
      new std::set<absl::string_view>{"3DNOW",
                                      "ADX",
                                      "AES",
                                      "AVX",
                                      "AVX2",
                                      "AVX512BW",
                                      "AVX512CD",
                                      "AVX512DQ",
                                      "AVX512ER",
                                      "AVX512F",
                                      "AVX512F",
                                      "AVX512PF",
                                      "AVX512VL",
                                      "AVX512_4FMAPS",
                                      "AVX512_4VNNIW",
                                      "AVX512_BITALG",
                                      "AVX512_IFMA",
                                      "AVX512_VBMI",
                                      "AVX512_VBMI2",
                                      "AVX512_VNNI",
                                      "AVX512_VPOPCNTDQ",
                                      "BMI1",
                                      "BMI2",
                                      "CET_IBT",
                                      "CET_SS",
                                      "CLDEMOTE",
                                      "CLMUL",
                                      "CLWB",
                                      "F16C",
                                      "FMA",
                                      "FPU",
                                      "FSGSBASE",
                                      "GFNI",
                                      "HLE",
                                      "INVPCID",
                                      "LZCNT",
                                      "MMX",
                                      "MOVDIR64B",
                                      "MOVDIRI",
                                      "MPX",
                                      "OSPKE",
                                      "PCLMULQDQ",
                                      "PREFETCHW",
                                      "RDPID",
                                      "RDRAND",
                                      "RDSEED",
                                      "RTM",
                                      "SGX1",
                                      "SGX2",
                                      "SHA",
                                      "SMAP",
                                      "SSE",
                                      "SSE2",
                                      "SSE3",
                                      "SSE4_1",
                                      "SSE4_2",
                                      "SSSE3",
                                      "VAES",
                                      "VPCLMULQDQ",
                                      "WAITPKG",
                                      "XSAVE",
                                      "XSAVEC",
                                      "XSAVEOPT",
                                      "XSS"};
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
       new RE2(R"(SIB\.base\s+\(r\):\s+Address of pointer\nSIB\.index\(r\))")},
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

}  // namespace

// We use a macro to avoid repeating the regex definition in FixFeature. We use
// the string constant to initialize LazyRE2 (which expects a const char*), so
// sadly we can't use a regular constexpr const char* constant.
#define EXEGESIS_AVX_REGEX_SOURCE                                      \
  "(AVX512BW|AVX512CD|AVX512DQ|AVX512ER|AVX512F|AVX512_BITALG|AVX512_" \
  "IFMA|AVX512_VNNI|AVX512PF|AVX512_VBMI|AVX512F|AVX512VL|GFNI|VAES|"  \
  "VPCLMULQDQ)"

// We want to normalize features to the set defined by GetValidFeatureSet or
// logical composition of them (several features separated by '&&' or '||')
// TODO(gchatelet): Move this to configuration file.
std::string FixFeature(std::string feature) {
  static const auto* const kReplacements =
      new absl::flat_hash_map<absl::string_view, absl::string_view>(
          {{"AESAVX", "AES && AVX"},
           {"AES AVX", "AES && AVX"},
           {"AVX512_VPOPCNTDQAVX512VL", "AVX512_VPOPCNTDQ && AVX512VL"},
           {"AVX512_VBMI2AVX512VL", "AVX512_VBMI2 && AVX512VL"},
           {"AVXGFNI", "AVX && GFNI"},
           {"Both AES andAVX flags", "AES && AVX"},
           {"Both PCLMULQDQ and AVX flags", "CLMUL && AVX"},
           {"HLE or RTM", "HLE || RTM"},
           {"PCLMULQDQ AVX", "CLMUL && AVX"},
           {"PCLMULQDQ", "CLMUL"},
           {"PREFETCHWT1", "3DNOW"},
           {"HLE1", "HLE"},
           // NOTE(ondrasej): PRFCHW was renamed to PREFETCHW in the November
           // 2018 version of the SDM. We always use the new name, but we want
           // to remain compatible with previous versions of the SDM.
           {"PRFCHW", "PREFETCHW"}});

  absl::StripAsciiWhitespace(&feature);
  RE2::GlobalReplace(&feature, R"([\n-])", "");
  // Matches a sequence of feature names with no separation between them.
  static const LazyRE2 kAvxRepeatedRegex = {EXEGESIS_AVX_REGEX_SOURCE "+"};
  // Matches a single feature name from the list above. The regexp is created by
  // droping the 'repeat once or more' operator from kAvxRepeatedRegex. Using
  // the regexp with repetition in RE2::Consume would lead to all but one
  // feature name being dropped.
  static const LazyRE2 kAvxRegex = {EXEGESIS_AVX_REGEX_SOURCE};
  if (RE2::FullMatch(feature, *kAvxRepeatedRegex)) {
    absl::string_view remainder(feature);
    absl::string_view piece;
    std::vector<absl::string_view> pieces;
    while (RE2::Consume(&remainder, *kAvxRegex, &piece)) {
      pieces.push_back(piece);
    }
    return absl::StrJoin(pieces, " && ");
  }
  absl::string_view replacement;
  if (gtl::FindCopy(*kReplacements, feature, &replacement)) {
    feature = std::string(replacement);
  }
  return feature;
}

#undef EXEGESIS_AVX_REGEX_SOURCE

namespace {

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

void ParseLeafSgxOpcodeInstructionCell(absl::string_view text,
                                       ParseContext* parse_context,
                                       InstructionProto* instruction) {
  static const LazyRE2 regex = {
      R"(EAX = ([A-F0-9]+)H\s*(ENCL[SUV])\[([A-Z]+)\])"};
  std::string eax_value;
  std::string main_instruction;
  std::string leaf_instruction;

  CHECK(RE2::FullMatch(text, *regex, &eax_value, &main_instruction,
                       &leaf_instruction))
      << "Unexpected text for OpCode/Instruction cell in SGX section: " << text;

  *instruction->mutable_llvm_mnemonic() = leaf_instruction;
  parse_context->AddLeafSgxInstruction(main_instruction, instruction);
  parse_context->AddRegisterOperandValue(instruction, "EAX", eax_value);
}

void ParseOpCodeInstructionCell(absl::string_view text,
                                bool save_instruction_ptr,
                                ParseContext* parse_context,
                                InstructionProto* instruction) {
  std::string mnemonic;
  if (RE2::PartialMatch(text, *kInstructionRegexp, &mnemonic)) {
    const size_t index_of_mnemonic = text.find(mnemonic);
    CHECK_NE(index_of_mnemonic, absl::string_view::npos);
    std::string opcode_text = std::string(text.substr(0, index_of_mnemonic));
    std::string instruction_text = std::string(text.substr(index_of_mnemonic));
    ParseVendorSyntax(std::move(instruction_text),
                      GetOrAddUniqueVendorSyntaxOrDie(instruction));
    instruction->set_raw_encoding_specification(
        FixEncodingSpecification(std::move(opcode_text)));
  } else {
    LOG(ERROR) << "Unable to separate opcode from instruction in " << text
               << ", setting to " << kUnknown;
    instruction->set_raw_encoding_specification(kUnknown);
  }

  if (save_instruction_ptr) {
    parse_context->AddMainSgxInstruction(mnemonic, instruction);
  }
}

void ParseCell(const InstructionTable::Column column, std::string text,
               ParseContext* parse_context, InstructionProto* instruction) {
  absl::StripAsciiWhitespace(&text);
  switch (column) {
    case InstructionTable::IT_OPCODE:
      instruction->set_raw_encoding_specification(
          FixEncodingSpecification(text));
      break;
    case InstructionTable::IT_INSTRUCTION:
      ParseVendorSyntax(text, GetOrAddUniqueVendorSyntaxOrDie(instruction));
      break;
    case InstructionTable::IT_OPCODE_INSTRUCTION: {
      switch (parse_context->instruction_type()) {
        case InstructionType::LEAF_SGX:
          ParseLeafSgxOpcodeInstructionCell(text, parse_context, instruction);
          break;
        case InstructionType::SGX:
          ParseOpCodeInstructionCell(text, /*save_instruction_ptr=*/true,
                                     parse_context, instruction);
          break;
        case InstructionType::VMX:
        case InstructionType::REGULAR:
          ParseOpCodeInstructionCell(text, /*save_instruction_ptr=*/false,
                                     parse_context, instruction);
          break;
        default:
          LOG(ERROR) << "Unknown instruction type "
                     << parse_context->GetInstructionTypeAsString();
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
      // Feature flags are not always consistent. FixFeature makes sure cleaned
      // is one of the valid feature values.
      const std::string cleaned = FixFeature(text);
      std::string* feature_name = instruction->mutable_feature_name();
      for (const absl::string_view piece : absl::StrSplit(cleaned, ' ')) {
        if (!feature_name->empty()) feature_name->append(" ");
        const bool is_logic_operator = piece == "&&" || piece == "||";
        if (is_logic_operator ||
            gtl::ContainsKey(GetValidFeatureSet(), piece)) {
          absl::StrAppend(feature_name, piece);
        } else {
          DLOG(ERROR) << "Raw feature text: [" << text << "]";
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

// Fixes the legacy mode/64-bit mode availability of VMX instructions. As of May
// 2019, the availability is not stored in the table. Instead, they are
// available in both modes, unless specified otherwise in the description.
void FixVmxInstructionAvailability(const ParseContext& parse_context,
                                   InstructionProto* instruction) {
  static constexpr char kIn64BitMode[] = "in 64-bit mode";
  static constexpr char kOutside64BitMode[] = "outside 64-bit mode";
  CHECK(instruction->feature_name().empty()) << instruction->DebugString();
  instruction->set_feature_name("VMX");
  instruction->set_available_in_64_bit(true);
  instruction->set_legacy_instruction(true);
  if (instruction->description().find(kIn64BitMode) != std::string::npos) {
    instruction->set_legacy_instruction(false);
  } else if (instruction->description().find(kOutside64BitMode) !=
             std::string::npos) {
    instruction->set_available_in_64_bit(false);
  }
}

void ParseInstructionTable(const SubSection& sub_section,
                           ParseContext* parse_context,
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
  if (columns.size() < 3) {
    LOG(ERROR) << "Discarding Instruction Table with less than 3 columns.";
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
    // NOTE(ondrasej): In some cases, a footnote marker at the end of the line
    // gets parsed as a separate column. Checking simply for a difference in the
    // number of blocks would stop the parsing here, discarding that instruction
    // and all instructions below it.
    if (row.blocks_size() < columns.size()) break;  // end of the table
    CHECK_LE(row.blocks_size(), columns.size()) << "Too many blocks in row:\n"
                                                << row.DebugString();
    parse_context->set_instruction_index(table->instructions_size());
    auto* instruction = table->add_instructions();
    int i = 0;
    for (const auto& block : row.blocks()) {
      ParseCell(table->columns(i++), block.text(), parse_context, instruction);
    }

    if (parse_context->IsVmx()) {
      FixVmxInstructionAvailability(*parse_context, instruction);
    }
  }
}

std::string GetRowText(const PdfTextTableRow& row) {
  return absl::StrJoin(row.blocks(), " ",
                       [](std::string* out, const PdfTextBlock& block) {
                         absl::StrAppend(out, block.text());
                       });
}

// Parses the given header row and returns the right OperandEncodingTableType.
OperandEncodingTableType GetOperandEncodingTableHeaderType(
    const PdfTextTableRow& row, ParseContext* parse_context) {
  static const LazyRE2 kHeaderRegex = {
      R"(Op/En|Operand[1234]|Tuple(Type)?|ImplicitRegisterOperands)"};
  static const LazyRE2 kLeafSgxHeaderRegex = {
      R"((Op/En|EAX|EBX|RAX|RBX|RCX|RDX))"};

  CHECK(parse_context != nullptr);
  bool has_tuple_type_column = false;
  for (const auto& block : row.blocks()) {
    std::string text = block.text();
    RemoveSpaceAndLF(&text);
    switch (parse_context->instruction_type()) {
      case InstructionType::LEAF_SGX: {
        std::string column_name;
        if (!RE2::FullMatch(text, *kLeafSgxHeaderRegex, &column_name)) {
          DLOG(ERROR) << "**** not matching on text: [" << text
                      << "] against sgx header regex";

          return OET_INVALID;
        }
        if ("Op/En" != column_name) {
          parse_context->AddRegister(std::move(column_name));
        }
      } break;
      default:
        if (text == "TupleType" || text == "Tuple") {
          has_tuple_type_column = true;
        }
        if (!RE2::FullMatch(text, *kHeaderRegex)) {
          DLOG(ERROR) << "**** not matching on text: [" << text
                      << "] against regular header regex";
          return OET_INVALID;
        }
    }
  }

  if (parse_context->IsLeafSgx()) {
    return OET_LEAF_SGX;
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

void ParseSgxOperandEncodingTableRow(const PdfTextTableRow& row,
                                     InstructionTable* table,
                                     ParseContext* parse_context) {
  // Op/En | EAX (| <other_reg>)*
  const int columns_count = row.blocks_size();
  int second_operand_index = -1;
  std::string eax_description;
  if (parse_context->GetRegistersCount() == columns_count - 1) {
    VLOG(1) << "EAX is in one cell";
    second_operand_index = 2;
    eax_description = row.blocks(1).text();
  } else if (parse_context->GetRegistersCount() == columns_count - 2) {
    // Sometimes the description for EAX column is split into two.
    VLOG(1) << "EAX is in two cells";
    second_operand_index = 3;
    eax_description =
        absl::StrCat(row.blocks(1).text(), "; ", row.blocks(2).text());
  } else {
    LOG(FATAL) << "Unexpected columns count of " << columns_count
               << " in row: " << GetRowText(row);
  }

  for (auto& leaf_sgx : *table->mutable_instructions()) {
    parse_context->AddRegisterOperandDescription(&leaf_sgx, "EAX",
                                                 eax_description);
    for (int index = second_operand_index; index < columns_count; ++index) {
      // `index - 1` because the table has more one more column than the number
      // of registers
      parse_context->AddRegisterOperandDescription(
          &leaf_sgx,
          parse_context->GetRegister(index - second_operand_index + 1),
          row.blocks(index).text());
    }
  }
}

// Extracts information from the Operand Encoding Table.
// For each row in the table we create an operand_encoding containing a
// crossreference_name and a list of operand_encoding_specs.
void ParseOperandEncodingTable(const SubSection& sub_section,
                               ParseContext* parse_context,
                               InstructionTable* table) {
  size_t column_count = 0;
  OperandEncodingTableType table_type = OET_INVALID;
  for (const auto& row : sub_section.rows()) {
    if (column_count == 0) {
      // Parsing the operand encoding table header, we just make sure the text
      // is valid but don't store any informations.
      column_count = row.blocks_size();
      table_type = GetOperandEncodingTableHeaderType(row, parse_context);
      CHECK_NE(table_type, OET_INVALID)
          << "Invalid operand header for instruction type "
          << parse_context->GetInstructionTypeAsString() << ": "
          << row.DebugString();
    } else {
      // Skipping redundant header.
      if (GetOperandEncodingTableHeaderType(row, parse_context) == table_type) {
        continue;
      }
      // Parse the SGX operand encodings. There might be more columns than in
      // the table header - when a register is used both as input and output,
      // they use a sub-column for each.
      if (table_type == OET_LEAF_SGX && row.blocks_size() >= column_count) {
        // Sanity check.
        if (!parse_context->IsLeafSgx()) {
          LOG(WARNING)
              << "See SGX-style operand encoding table in non-SGX chapters";
        }
        ParseSgxOperandEncodingTableRow(row, table, parse_context);
        return;
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
void PairOperandEncodings(ParseContext* parse_context,
                          InstructionSection* section) {
  auto* table = section->mutable_instruction_table();
  std::map<std::string, const InstructionTable::OperandEncodingCrossref*>
      mapping;
  std::set<std::string> duplicated_crossreference;
  for (const auto& operand_encoding : table->operand_encoding_crossrefs()) {
    const std::string& cross_reference = operand_encoding.crossreference_name();
    if (!gtl::InsertIfNotPresent(&mapping, cross_reference,
                                 &operand_encoding)) {
      LOG(ERROR) << "Duplicated Operand Encoding Scheme for " << section->id()
                 << ", this will result in UNKNOWN operand encoding sheme";
      gtl::InsertIfNotPresent(&duplicated_crossreference, cross_reference);
    }
  }

  // SGX leaf instructions operand tables use a slightly different format, and
  // they are parsed during the parsing of the instruction itself.
  if (mapping.empty() && parse_context->IsLeafSgx()) {
    return;
  }

  // VMX instructions don't have encoding table.
  if (mapping.empty() && parse_context->IsVmx()) {
    for (auto& instruction : *table->mutable_instructions()) {
      auto* const vendor_syntax = GetOrAddUniqueVendorSyntaxOrDie(&instruction);
      for (int i = 0; i < vendor_syntax->operands_size(); ++i) {
        auto* const operand = vendor_syntax->mutable_operands(i);
        operand->set_usage(InstructionOperand::USAGE_READ_WRITE);
      }
    }
    return;
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
    if (!gtl::ContainsKey(mapping, encoding_scheme)) {
      LOG(ERROR) << "Unable to find crossreference " << encoding_scheme
                 << " in Operand Encoding Table";
      continue;
    }
    const auto* encoding = gtl::FindOrDie(mapping, encoding_scheme);
    auto* const vendor_syntax = GetOrAddUniqueVendorSyntaxOrDie(&instruction);

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
                        ParseContext* parse_context,
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
        ParseInstructionTable(sub_section, parse_context, instruction_table);
        break;
      case SubSection::INSTRUCTION_OPERAND_ENCODING:
        ParseOperandEncodingTable(sub_section, parse_context,
                                  instruction_table);
        break;
      default:
        break;
    }
    sub_section.Swap(section->add_sub_sections());
  }
  PairOperandEncodings(parse_context, section);
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

// Returns true if the rows match the pattern of the first page of a VMX
// instruction group section in the layout from before May 2019.
bool MatchesVmxFirstPageBeforeMay2019(
    const std::vector<const PdfTextTableRow*>& rows,
    absl::string_view instruction_name) {
  static constexpr int kTableHeaderRow = 1;
  static constexpr int kFirstTableRow = 2;
  static constexpr int kOpCodeColumn = 0;
  static constexpr int kInstructionColumn = 1;
  static constexpr int kDescriptionColumn = 2;

  CHECK_EQ(3, rows.size());

  // title: <ID><DASH><TEXT>
  // table-header:   Opcode      Instruction   Description
  // row_0:          XX( XX)*    <ID>( <OP>)*   <TEXT>
  const auto& header_blocks = rows[kTableHeaderRow]->blocks();
  return header_blocks.size() == 3 &&
         rows[kFirstTableRow]->blocks_size() == 3 &&
         header_blocks[kOpCodeColumn].text() == "Opcode" &&
         header_blocks[kInstructionColumn].text() == "Instruction" &&
         header_blocks[kDescriptionColumn].text() == "Description" &&
         // check that the instruction in the table matches the title
         absl::StartsWith(
             rows[kFirstTableRow]->blocks(kInstructionColumn).text(),
             instruction_name);
}

// Returns true if the rows match the pattern of the first page of a VMX
// instruction group section in the layout from since May 2019.
bool MatchesVmxFirstPageSinceMay2019(
    const std::vector<const PdfTextTableRow*>& rows,
    absl::string_view instruction_name) {
  static constexpr int kTableHeaderRow = 1;
  static constexpr int kFirstTableRow = 2;
  static constexpr int kOpCodeColumn = 0;
  static constexpr int kOpEnColumn = 1;
  static constexpr int kDescriptionColumn = 2;

  CHECK_EQ(3, rows.size());

  // title: <ID><DASH><TEXT>
  // table-header: | Opcode/      | Op/En | Description
  //               | Instruction  |       |
  //               -----------------------------------------------
  // row_0:          *first instruction*
  const auto& header_blocks = rows[kTableHeaderRow]->blocks();
  return header_blocks.size() == 3 &&
         rows[kFirstTableRow]->blocks_size() == 3 &&
         (header_blocks[kOpCodeColumn].text() == "Opcode/ \nInstruction" ||
          header_blocks[kOpCodeColumn].text() == "Opcode/\nInstruction") &&
         header_blocks[kOpEnColumn].text() == "Op/En" &&
         header_blocks[kDescriptionColumn].text() == "Description";
}

// Returns true if the rows match the pattern of the first page of a VMX
// instruction group section.
//
// NOTE(ondrasej): With the May 2019 version of the SDM, the layout of the VMX
// instruction page changed and resembles more the layout of instructions in the
// main section of the SDM.
//
// rows: vector containing the first three rows of the page.
// instruction_name: name of the first instruction in this group.
bool MatchesVmxFirstPage(const std::vector<const PdfTextTableRow*>& rows,
                         absl::string_view instruction_name) {
  return MatchesVmxFirstPageBeforeMay2019(rows, instruction_name) ||
         MatchesVmxFirstPageSinceMay2019(rows, instruction_name);
}

bool MatchesSgxInstruction(absl::string_view cell_text,
                           absl::string_view instruction_name, bool* is_leaf) {
  CHECK(is_leaf != nullptr);
  *is_leaf = false;
  return absl::EndsWith(cell_text, instruction_name) ||
         (*is_leaf = absl::EndsWith(cell_text,
                                    absl::StrCat("[", instruction_name, "]")));
}

// Returns true if the rows match the pattern of the first page of an SGX
// instruction group section.
// rows: vector containing the first three rows of the page.
// instruction_name: name of the first instruction in this group.
bool MatchesSgxFirstPage(const std::vector<const PdfTextTableRow*>& rows,
                         absl::string_view instruction_name, bool* is_leaf) {
  static constexpr int kTableHeaderRow = 1;
  static constexpr int kFirstTableRow = 2;
  static constexpr int kOpCodeColumn = 0;
  static constexpr int kOpEnColumn = 1;
  static constexpr int kDescriptionColumn = 4;

  CHECK(is_leaf != nullptr);
  CHECK_EQ(3, rows.size());

  // title: <ID><DASH><TEXT>
  // table-header:   Opcode/Instruction  Op/En  64/32   CPUID   Description
  // row_0:          XX XX XX XX          XX     X/X      XX    <TEXT>
  //                 <ID>
  return rows[kTableHeaderRow]->blocks_size() == 5 &&
         rows[kFirstTableRow]->blocks_size() == 5 &&
         absl::StartsWith(rows[kTableHeaderRow]->blocks(kOpCodeColumn).text(),
                          "Opcode/") &&
         absl::StartsWith(rows[kTableHeaderRow]->blocks(kOpEnColumn).text(),
                          "Op/En") &&
         absl::StartsWith(
             rows[kTableHeaderRow]->blocks(kDescriptionColumn).text(),
             "Description") &&
         // Checks that the instruction in the table matches the title.
         MatchesSgxInstruction(
             rows[kFirstTableRow]->blocks(kOpCodeColumn).text(),
             instruction_name, is_leaf);
}

// Returns the first instruction in this group. If the group only contains one
// instruction, then the group name is the same as the first-instruction's name.
// If the group contains more than one instruction, then the group name will
// consist of all the instructions' name, separated by  a slash '/'.
//
// Eg., "VMLAUNCH/VMRESUME—Launch/ResumeVirtual Machine" in V3-Chapter30.
//       This function will return "VMLAUNCH"
absl::string_view GetFirstInstructionInGroup(absl::string_view group_name) {
  return group_name.substr(0, group_name.find_first_of('/'));
}

// If this page matches the pattern of the first page for an instruction
// in the VMX INSTRUCTION REFERENCE or SGX INSTRUCTION REFERENCE section in
// the SDM, returns true and puts the instruction's name into the output
// parameter instruction_name.
bool MatchesFirstPagePattern(const PdfPage& page, const bool is_sgx,
                             bool* is_leaf,
                             std::string* const instruction_group_name) {
  const PdfTextBlock* const name_cell = GetCellOrNull(page, 1, 0);
  if (name_cell == nullptr) {
    return false;
  }
  auto dash_pos = name_cell->text().find_first_of('-');
  if (dash_pos == std::string::npos) {
    return false;
  }
  const auto rows = GetPageBodyRows(page, kPageMargin, 3);
  if (rows.size() != 3) {
    return false;
  }
  std::string group_name = name_cell->text().substr(0, dash_pos);
  const absl::string_view first_instruction_name =
      GetFirstInstructionInGroup(group_name);
  if ((is_sgx && MatchesSgxFirstPage(rows, first_instruction_name, is_leaf)) ||
      (!is_sgx && MatchesVmxFirstPage(rows, first_instruction_name))) {
    *instruction_group_name = std::move(group_name);
    return true;
  }
  return false;
}

// Returns true if we the given page matches the pattern of the first page in
// a section.
bool SeesNewSection(const PdfPage& page,
                    absl::string_view section_number_prefix,
                    absl::string_view section_name_prefix) {
  const PdfTextBlock* const section_number = GetCellOrNull(page, 1, 0);

  if (section_number == nullptr) {
    return false;
  }
  if (absl::StartsWith(section_number->text(), section_number_prefix)) {
    const PdfTextBlock* const section_name = GetCellOrNull(page, 1, 1);
    if (section_name == nullptr) {
      return false;
    }
    return absl::StartsWith(section_name->text(), section_name_prefix);
  }
  return false;
}

// Adds the value vector into the map if the vector is not empty,
// also warns if there's an existing entry with the same key
void AddAndWarn(
    absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>* const
        id_to_pages,
    const std::string& name, InstructionType type, Pages value) {
  if (!value.empty()) {
    if (id_to_pages->count(name) != 0) {
      // TODO(user): could this happen? should we just append to it then?
      LOG(WARNING) << "Overwriting existing instruction pages for [" << name
                   << "].";
    }

    (*id_to_pages)[name] = std::make_pair(type, std::move(value));
  }
}

// Starting at page_idx, collects all pages for the current
// instruction, if there is any.
//
// Pre-condition: page_idx is pointing at a page in the VMX instruction
// reference or SGX instruction reference chapers.
// Returns: page index of the next instruction's first page
int CollectVmxOrSgxInstructions(
    const exegesis::pdf::PdfDocument& pdf, int page_idx, bool is_sgx,
    absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>* const
        id_to_pages) {
  const auto& first_page = pdf.pages(page_idx);
  std::string instruction_name;
  bool is_leaf = false;
  if (MatchesFirstPagePattern(first_page, is_sgx, &is_leaf,
                              &instruction_name)) {
    const InstructionType type =
        !is_sgx ? InstructionType::VMX
                : is_leaf ? InstructionType::LEAF_SGX : InstructionType::SGX;
    Pages result;
    result.push_back(&first_page);
    ++page_idx;

    for (; page_idx < pdf.pages_size(); ++page_idx) {
      std::string new_name;
      const auto& cur_page = pdf.pages(page_idx);
      // We can't tell when a section ends. We can only determine that by
      // looking ahead for the start of the next thing (either the next
      // instruction or a new section entirely).
      bool next_leaf = false;
      if (MatchesFirstPagePattern(cur_page, is_sgx, &next_leaf, &new_name)) {
        break;
      } else if (SeesNewSection(cur_page, is_sgx ? "40." : "30.",
                                is_sgx ? "INTEL® SGX" : "VM INSTRUCTION")) {
        break;
      }
      result.push_back(&cur_page);
    }
    AddAndWarn(id_to_pages, instruction_name, type, std::move(result));
    return page_idx;
  }
  // If didn't see anything useful, move past this page.
  return page_idx + 1;
}

// Returns true if this page matches the pattern of the first page for an
// instruction section in the Instruction-Set-Extension document.
// page: the current page.
// group_name: the output parameter - if this page is the first page, the
// group name will be returned via this.
bool MatchesFirstPageInExtension(const PdfPage& page,
                                 std::string* const group_name) {
  static constexpr int kTableHeaderRow = 1;
  static constexpr int kFirstTableRow = 2;
  static constexpr int kOpCodeColumn = 0;
  static constexpr int kOpEnColumn = 1;
  static constexpr int kDescriptionColumn = 4;

  CHECK(group_name != nullptr);
  const PdfTextBlock* const name_cell =
      GetCellOrNull(page, /*row=*/1, /*col=*/0);
  if (name_cell == nullptr) {
    return false;
  }
  const int dash_pos = name_cell->text().find_first_of('-');
  if (dash_pos == std::string::npos) {
    // Did not get expected data - skipping this page.
    return false;
  }

  const std::vector<const PdfTextTableRow*> first_three_rows =
      GetPageBodyRows(page, kPageMargin, 3);
  if (first_three_rows.size() != 3) {
    // Did not get expected data - skipping this page.
    return false;
  }
  std::string group_name_string = name_cell->text().substr(0, dash_pos);
  const absl::string_view first_instruction_name =
      GetFirstInstructionInGroup(group_name_string);

  // title: <ID><DASH><TEXT>
  // table-header: |Opcode/     |Op/ |64/32 bit    |CPUID feature |Description
  //               |Instruction |En  |Mode Support |Flag          |
  //               -------------------------------------------------------------
  // row_0:         *first_instruction*
  const auto* const header_row = first_three_rows[kTableHeaderRow];
  const auto* const first_table_row = first_three_rows[kFirstTableRow];
  if (header_row->blocks_size() == 5 && first_table_row->blocks_size() == 5 &&
      absl::StartsWith(header_row->blocks(kOpCodeColumn).text(), "Opcode/") &&
      absl::StartsWith(header_row->blocks(kOpEnColumn).text(), "Op/") &&
      absl::StartsWith(header_row->blocks(kDescriptionColumn).text(),
                       "Description") &&
      absl::StrContains(first_table_row->blocks(kOpCodeColumn).text(),
                        first_instruction_name)) {
    *group_name = std::move(group_name_string);
    return true;
  }
  return false;
}

int CollectFromInstructionSetExtension(
    const PdfDocument& pdf, int page_idx,
    absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>* const
        id_to_pages) {
  CHECK(id_to_pages != nullptr);
  std::string group_name;
  const PdfPage& first_page = pdf.pages(page_idx);
  if (MatchesFirstPageInExtension(first_page, &group_name)) {
    Pages result = {&first_page};
    ++page_idx;
    for (; page_idx < pdf.pages_size(); ++page_idx) {
      std::string next_group;
      const auto& cur_page = pdf.pages(page_idx);
      const std::string first_line =
          GetCellTextOrEmpty(pdf.pages(page_idx), 0, 0);
      // If we are no longer in Instruction-Set-Reference or if we see the
      // start of a new instruction, then break.
      if (!absl::StartsWith(first_line, kInstructionSetRef) ||
          !MatchesFirstPageInExtension(cur_page, &next_group)) {
        break;
      }
      result.push_back(&cur_page);
    }
    AddAndWarn(id_to_pages, group_name, InstructionType::REGULAR,
               std::move(result));
    return page_idx;
  }
  // If we didn't see expected data, skip this page.
  return page_idx + 1;
}

// Starting at page_idx, collect all pages for the current instruction, if any
// page.
//
// Pre-condition: page_idx is pointing at a page probably in V2 or the
// extension manual.
// Returns: page index of the next instruction
int CollectFromTheRest(
    const PdfDocument& pdf, int page_idx,
    absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>* const
        id_to_pages) {
  static constexpr float kMaxGroupNameVerticalPosition = 500.0f;

  const auto& page = pdf.pages(page_idx);
  const PdfTextBlock* const name_cell = GetCellOrNull(page, 1, 0);
  if (name_cell != nullptr &&
      name_cell->bounding_box().top() <= kMaxGroupNameVerticalPosition) {
    const std::string& footer_section_name = GetFooterSectionName(page);
    if (SameInstructionName(name_cell->text(), footer_section_name)) {
      Pages result;
      for (; page_idx < pdf.pages_size(); ++page_idx) {
        const auto& page = pdf.pages(page_idx);
        if (!IsPageInstruction(page, footer_section_name)) break;
        result.push_back(&page);
      }

      AddAndWarn(id_to_pages, footer_section_name, InstructionType::REGULAR,
                 std::move(result));
      return page_idx;
    } else if (absl::StartsWith(footer_section_name, "Ref. #")) {
      // In V2, sll instruction-reference pages have a footer matching the
      // title, but in the Instruction Set Extension, the footer only contains
      // text like "Ref. #..."
      // Therefore, we distinguish V2 and the extension by having different
      // expections for the footer.
      return CollectFromInstructionSetExtension(pdf, page_idx, id_to_pages);
    }
  }
  // If didn't see anything, move past this page.
  return page_idx + 1;
}

// Returns a map of instruction-name to a vector of pairs of a bool flag and
// pages for it. The flag is true IFF the pages are for SGX instructions.
absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>
CollectInstructionPages(const exegesis::pdf::PdfDocument& pdf) {
  absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>
      instruction_group_id_to_pages;
  int i = 0;
  while (i < pdf.pages_size()) {
    const std::string first_line = GetCellTextOrEmpty(pdf.pages(i), 0, 0);
    if (absl::StartsWith(first_line, kVmxInstructionRef)) {
      i = CollectVmxOrSgxInstructions(pdf, i, /*is_sgx=*/false,
                                      &instruction_group_id_to_pages);
    } else if (absl::StartsWith(first_line, kSgxInstructionRef)) {
      i = CollectVmxOrSgxInstructions(pdf, i, /*is_sgx=*/true,
                                      &instruction_group_id_to_pages);
    } else if (absl::StartsWith(first_line, kInstructionSetRef)) {
      // Volume 2
      i = CollectFromTheRest(pdf, i, &instruction_group_id_to_pages);
    } else {
      // Found nothing on this page, move to the next.
      ++i;
    }
  }
  return instruction_group_id_to_pages;
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
  absl::node_hash_map<std::string, std::pair<InstructionType, Pages>>
      instruction_group_id_to_pages = CollectInstructionPages(pdf);

  ParseContext parse_context;
  // Now processing instruction pages
  for (const auto& id_pages_pair : instruction_group_id_to_pages) {
    parse_context.Reset();
    parse_context.set_instruction_type(id_pages_pair.second.first);
    parse_context.set_section_index(sdm_document.instruction_sections_size());
    InstructionSection section;
    const auto& group_id = id_pages_pair.first;
    const auto& pages = id_pages_pair.second.second;
    LOG(INFO) << "Processing section id " << group_id << " pages "
              << pages.front()->number() << "-" << pages.back()->number();
    section.set_id(group_id);
    ProcessSubSections(ExtractSubSectionRows(pages), &parse_context, &section);
    if (section.instruction_table().instructions_size() == 0) {
      LOG(WARNING) << "Empty instruction table, skipping the section";
      continue;
    }
    section.Swap(sdm_document.add_instruction_sections());
  }

  parse_context.RelocateSgxLeafInstructions(&sdm_document);
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
