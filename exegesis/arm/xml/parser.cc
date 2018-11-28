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

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/arm/xml/docvars.h"
#include "exegesis/arm/xml/docvars.pb.h"
#include "exegesis/arm/xml/markdown.h"
#include "exegesis/arm/xml/parser.pb.h"
#include "exegesis/proto/instruction_encoding.pb.h"
#include "exegesis/util/xml/xml_util.h"
#include "file/base/path.h"
#include "glog/logging.h"
#include "net/proto2/util/public/repeated_field_util.h"
#include "src/google/protobuf/repeated_field.h"
#include "tinyxml2.h"
#include "util/task/canonical_errors.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace arm {
namespace xml {

namespace {

using ::exegesis::util::Annotate;
using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::IsNotFound;
using ::exegesis::util::NotFoundError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;
using ::exegesis::xml::DebugString;
using ::exegesis::xml::FindChild;
using ::exegesis::xml::FindChildren;
using ::exegesis::xml::GetStatus;
using ::exegesis::xml::ReadAttribute;
using ::exegesis::xml::ReadIntAttribute;
using ::exegesis::xml::ReadIntAttributeOrDefault;
using ::exegesis::xml::ReadSimpleText;
using ::google::protobuf::RepeatedPtrField;
using ::tinyxml2::XMLDocument;
using ::tinyxml2::XMLElement;
using ::tinyxml2::XMLNode;

using BitRange = FixedSizeInstructionLayout::BitRange;
using BitPattern = BitRange::BitPattern;

// Parses the brief & authored descriptions of the given <desc> XML node.
Status ParseDescriptions(const XMLNode* desc, XmlInstruction* instruction) {
  CHECK(desc != nullptr);
  CHECK(instruction != nullptr);
  const XMLElement* brief = desc->FirstChildElement("brief");
  instruction->set_brief_description(ExportToMarkdown(brief));
  const XMLElement* authored = desc->FirstChildElement("authored");
  instruction->set_authored_description(ExportToMarkdown(authored));
  return OkStatus();
}

// Parses a given string as a bit pattern element, failing if it doesn't comply
// with the specified pattern type, and possibly updates the pattern type if it
// was still UNDECIDED and the given bit forces the decision.
// Valid for all pattern types (doesn't change type):
//   >  "x" means "don't care", typically to represent a variable parameter.
// Valid only for MATCHING (or transforming UNDECIDED to MATCHING).
//   >  "1" represents a constant bit set to one.
//   >  "0" and "z" represent a constant bit set to zero. The tiny difference of
//      semantics resides in the fact that "z" is used repeated over a range of
//      bits that conveys a "(0)" meaning (see below) at a entire field level.
//   >  "(0)" and "(1)" mean "should be zero/one and it is *unpredictable* what
//      happens if they are not" (see ttps://goo.gl/AXFBZj for more context).
// Valid only for NOT_MATCHING (or transforming UNDECIDED to NOT_MATCHING).
//   >  "N" means that the bit isn't one.
//   >  "Z" means that the bit isn't zero.
StatusOr<BitPattern::Bit> ParseBit(const std::string& bit, PatternType* type) {
  CHECK(type != nullptr);
  if (bit.empty() || bit == "x") {
    return BitPattern::VARIABLE;
  }
  if (*type == PatternType::UNDECIDED || *type == PatternType::MATCHING) {
    if (bit == "1" || bit == "(1)") {
      *type = PatternType::MATCHING;
      return BitPattern::CONSTANT_ONE;
    } else if (bit == "0" || bit == "(0)" || bit == "z") {
      *type = PatternType::MATCHING;
      return BitPattern::CONSTANT_ZERO;
    }
  }
  if (*type == PatternType::UNDECIDED || *type == PatternType::NOT_MATCHING) {
    if (bit == "N") {
      *type = PatternType::NOT_MATCHING;
      return BitPattern::CONSTANT_ONE;
    } else if (bit == "Z") {
      *type = PatternType::NOT_MATCHING;
      return BitPattern::CONSTANT_ZERO;
    }
  }
  return InvalidArgumentError(absl::StrCat(
      "Unrecognized bit '", bit, "' for pattern ", PatternType_Name(*type)));
}

// Parses a field-wise constraint string like "!= 0000" or "!= 111x" into
// individual constraint bits. Returns an empty vector if there is no constraint
// or fails if the constraint is non-empty but malformed.
StatusOr<std::vector<std::string>> ParsePattern(const std::string& constraint) {
  if (constraint.empty()) return std::vector<std::string>{};
  absl::string_view raw_pattern(constraint);

  if (!absl::ConsumePrefix(&raw_pattern, "!=")) {
    return InvalidArgumentError(absl::StrCat("Invalid constraint '", constraint,
                                             "', expected leading '!='"));
  }
  raw_pattern = absl::StripAsciiWhitespace(raw_pattern);
  std::vector<std::string> pattern(raw_pattern.size());
  for (int i = 0; i < raw_pattern.size(); ++i) {
    const char bit = raw_pattern[i];
    switch (bit) {
      case '1':
        pattern[i] = "N";
        break;
      case '0':
        pattern[i] = "Z";
        break;
      case 'x':
        pattern[i] = "x";
        break;
      default:
        return InvalidArgumentError(absl::StrCat(
            "Invalid bit '", std::string(1, bit), "' in '", constraint, "'"));
    }
  }
  return pattern;
}

// Parses the raw bits from a XML <box> node as strings, not decoding semantics.
Status ParseRawBits(XMLElement* box, RawInstructionLayout::Field* field) {
  int bit_idx = 0;
  for (XMLElement* c : FindChildren(box, "c")) {
    const int span = ReadIntAttributeOrDefault(c, "colspan", 1);
    if (span <= 0)
      return InvalidArgumentError(absl::StrCat("Invalid span ", span));
    const std::string bit = ReadSimpleText(c);
    for (int span_idx = 0; span_idx < span; ++span_idx, ++bit_idx) {
      if (bit_idx > field->bits_size() - 1) {
        return InvalidArgumentError("Oversized bit initialization pattern");
      }
      // Don't force any undefined bit to preserve any parent-defined value.
      if (!bit.empty()) field->set_bits(bit_idx, bit);
    }
  }
  return OkStatus();
}

// Finds any field in fields exactly matching the specified [msb:lsb] range,
// or returns NOT_FOUND if no such field is found. Fails with an other error if:
// - the given range matches more than one field.
// - the given range overlaps with a field but has misaligned boundaries.
StatusOr<RawInstructionLayout::Field*> FindField(
    RepeatedPtrField<RawInstructionLayout::Field>* fields, const int msb,
    const int lsb) {
  RawInstructionLayout::Field* found = nullptr;
  for (auto& field : *fields) {
    const bool exact_overlap = field.msb() == msb && field.lsb() == lsb;
    const bool loose_overlap = field.lsb() <= msb && lsb <= field.msb();
    if (exact_overlap) {
      // Allow only a single exact correspondence.
      if (found != nullptr) {
        return InvalidArgumentError(absl::StrCat(
            "Multiple matches for bit range [", msb, ":", lsb, "]"));
      }
      found = &field;
    } else if (loose_overlap) {
      return InvalidArgumentError(
          absl::StrCat("Misalignment of bit range [", msb, ":", lsb, "]",
                       absl::StrCat(" over field '", field.name(), "' [",
                                    field.msb(), ":", field.lsb(), "]")));
    }
  }
  if (found) return found;
  return NotFoundError(
      absl::StrCat("No field matching bit range [", msb, ":", lsb, "]"));
}

// Detects any field-wise constraint like "!= 0000", "!= 111x", ...
// Returns OK and updates field accordingly if a valid constraint is correctly
// parsed. Returns NOT_FOUND if no constraint is detected, or any other error
// if a constraint exists but can't be decoded.
Status DetectConstraint(XMLElement* box, RawInstructionLayout::Field* field) {
  const std::string constraint = ReadAttribute(box, "constraint");
  const auto pattern = ParsePattern(constraint);
  if (!pattern.ok()) return pattern.status();
  const std::vector<std::string>& pattern_bits = pattern.ValueOrDie();
  if (pattern_bits.empty()) return NotFoundError("No constraint detected");

  // Validate pattern size.
  const int width = field->msb() - field->lsb() + 1;
  if (pattern_bits.size() != width) {
    return InvalidArgumentError(
        absl::StrCat("Constraint size mismatch: expected pattern holding ",
                     width, " bits but got constraint '", constraint, "'"));
  }

  // Pattern specifications always totally override any pre-existing base data.
  field->clear_bits();
  for (const auto& bit : pattern_bits) field->add_bits(bit);

  return OkStatus();
}

// Merges an existing instruction layout with additional information from the
// given node. This is mainly used to specialize instruction encodings from the
// generic layout of the base instruction class.
StatusOr<RawInstructionLayout> MergeInstructionLayout(
    XMLNode* node, const RawInstructionLayout& base) {
  CHECK(node != nullptr);

  RawInstructionLayout result = base;
  for (XMLElement* box : FindChildren(node, "box")) {
    // Parse and validate the bit range itself.
    const auto msb_found = ReadIntAttribute(box, "hibit");
    if (!msb_found.ok()) return msb_found.status();
    const int msb = msb_found.ValueOrDie();
    const int width = ReadIntAttributeOrDefault(box, "width", 1);
    const int lsb = msb - width + 1;
    if (msb > 31 || width < 1 || lsb < 0) {
      return InvalidArgumentError(
          absl::StrCat("Invalid bit range: [", msb, ":", lsb, "]"));
    }

    RawInstructionLayout::Field* field = nullptr;
    auto base_field = FindField(result.mutable_fields(), msb, lsb);
    if (base_field.ok()) {
      // Detect any pre-existing field exactly overridden by this new range...
      field = base_field.ValueOrDie();
    } else if (IsNotFound(base_field.status())) {
      // ... or add a new field if no override was found.
      field = result.add_fields();
    } else {
      return base_field.status();
    }

    field->set_name(ReadAttribute(box, "name"));
    field->set_msb(msb);
    field->set_lsb(lsb);

    // If a constraint is detected, skip parsing the field's XML subtree.
    const Status constraint_detection = DetectConstraint(box, field);
    if (constraint_detection.ok()) continue;
    // But don't fail if there's simply no constraint.
    if (!IsNotFound(constraint_detection)) return constraint_detection;

    // Pre-fill the range assuming all bits are undefined until properly parsed.
    while (field->bits_size() < width) field->add_bits("");

    // Parse individual raw bit values if present.
    const Status status = ParseRawBits(box, field);
    if (!status.ok()) return Annotate(status, DebugString(box));
  }

  // Reorder everything to ensure fields are in order (necessary when merging).
  Sort(result.mutable_fields(), [](const RawInstructionLayout::Field* a,
                                   const RawInstructionLayout::Field* b) {
    return a->msb() > b->msb();
  });
  return result;
}

// Parses a base instruction layout from the given <regdiagram> XML node.
StatusOr<RawInstructionLayout> ParseBaseInstructionLayout(
    XMLElement* regdiagram) {
  CHECK(regdiagram != nullptr);
  if (ReadAttribute(regdiagram, "form") != "32") {
    return FailedPreconditionError(
        absl::StrCat("Unexpected regdiagram form:\n", DebugString(regdiagram)));
  }
  const std::string name = ReadAttribute(regdiagram, "psname");
  if (name.empty()) {
    return NotFoundError(
        absl::StrCat("Missing psname:\n", DebugString(regdiagram)));
  }
  RawInstructionLayout result;
  result.set_name(name);
  return MergeInstructionLayout(regdiagram, result);
}

// Performs the actual bit pattern parsing from the raw RawInstructionLayout,
// after all partial segments have been merged together.
// Assumes all invariants of RawInstructionLayout are respected in the input.
StatusOr<FixedSizeInstructionLayout> ParseFixedSizeInstructionLayout(
    const RawInstructionLayout& raw) {
  FixedSizeInstructionLayout layout;
  layout.set_form_name(raw.name());

  for (const auto& field : raw.fields()) {
    BitRange* bit_range = layout.add_bit_ranges();
    // Copy base data.
    bit_range->set_name(field.name());
    bit_range->set_msb(field.msb());
    bit_range->set_lsb(field.lsb());
    // Parse raw bit specs as a pattern, determining its type on the fly.
    PatternType type = PatternType::UNDECIDED;
    auto* pattern = bit_range->mutable_pattern();
    for (int i = 0; i < field.bits_size(); ++i) {
      const auto parsed = ParseBit(field.bits(i), &type);
      if (!parsed.ok()) {
        return Annotate(
            parsed.status(),
            absl::StrCat(
                "while parsing bit # ", i, " '", field.bits(i), "' of field '",
                field.name(), "' ",
                absl::StrCat("['", absl::StrJoin(field.bits(), "','"), "'] ",
                             "in a ", PatternType_Name(type), " pattern")));
      }
      pattern->add_bits(parsed.ValueOrDie());
    }
    if (type == PatternType::NOT_MATCHING) {
      // Don't just swap the fields - as it's a oneof access order is important.
      const auto tmp = bit_range->pattern();
      *bit_range->mutable_not_pattern() = tmp;
    }
  }
  return layout;
}

// Parses the assembly template from the given <asmtemplate> XML node.
StatusOr<AsmTemplate> ParseAsmTemplate(XMLNode* asmtemplate) {
  CHECK(asmtemplate != nullptr);
  AsmTemplate result;

  for (XMLElement* element : FindChildren(asmtemplate, nullptr)) {
    const std::string tag = element->Name() ? std::string(element->Name()) : "";
    if (tag == "text") {
      auto* piece = result.add_pieces();
      piece->set_text(ReadSimpleText(element));
    } else if (tag == "a") {
      auto* piece = result.add_pieces();
      auto* symbol = piece->mutable_symbol();
      symbol->set_id(ReadAttribute(element, "link"));
      symbol->set_label(ReadSimpleText(element));
      symbol->set_description(ReadAttribute(element, "hover"));
      // field & explanation will get populated later by ParseExplanations().
    }
  }
  if (result.pieces().empty()) return NotFoundError("Empty ASM template");
  return result;
}

// Parses all instruction encodings from the given <iclass> XML node.
StatusOr<RepeatedPtrField<InstructionEncoding>> ParseInstructionEncodings(
    XMLNode* iclass) {
  CHECK(iclass != nullptr);
  RepeatedPtrField<InstructionEncoding> result;

  const auto regdiagram = FindChild(iclass, "regdiagram");
  if (!regdiagram.ok()) return regdiagram.status();
  const auto base_instruction_layout =
      ParseBaseInstructionLayout(regdiagram.ValueOrDie());
  if (!base_instruction_layout.ok()) return base_instruction_layout.status();

  for (XMLElement* encoding : FindChildren(iclass, "encoding")) {
    auto* encoding_proto = result.Add();
    // Unlike <iclass> elements, here the "name" attribute acts more like an id,
    // and the human-friendly name is instead stored as the "label" attribute.
    encoding_proto->set_id(ReadAttribute(encoding, "name"));
    std::string label = ReadAttribute(encoding, "label");
    const std::string suffix = ReadAttribute(encoding, "bitdiffs");
    if (!suffix.empty()) absl::StrAppend(&label, " (", suffix, ")");
    encoding_proto->set_name(label);

    const auto docvars = FindChild(encoding, "docvars");
    if (!docvars.ok()) return docvars.status();
    const auto docvars_proto = ParseDocVars(docvars.ValueOrDie());
    if (!docvars_proto.ok()) return docvars_proto.status();
    *encoding_proto->mutable_docvars() = docvars_proto.ValueOrDie();

    const auto asmtemplate = FindChild(encoding, "asmtemplate");
    if (!asmtemplate.ok()) return asmtemplate.status();
    const auto asmtemplate_proto = ParseAsmTemplate(asmtemplate.ValueOrDie());
    if (!asmtemplate_proto.ok()) return asmtemplate_proto.status();
    *encoding_proto->mutable_asm_template() = asmtemplate_proto.ValueOrDie();

    const auto raw_instruction_layout =
        MergeInstructionLayout(encoding, base_instruction_layout.ValueOrDie());
    if (!raw_instruction_layout.ok()) return raw_instruction_layout.status();
    const auto instruction_layout =
        ParseFixedSizeInstructionLayout(raw_instruction_layout.ValueOrDie());
    if (!instruction_layout.ok()) return instruction_layout.status();
    *encoding_proto->mutable_instruction_layout() =
        instruction_layout.ValueOrDie();
  }
  return result;
}

// Parses all instruction classes from the given <classes> XML node.
StatusOr<RepeatedPtrField<InstructionClass>> ParseInstructionClasses(
    XMLNode* classes) {
  CHECK(classes != nullptr);
  RepeatedPtrField<InstructionClass> result;

  for (XMLElement* iclass : FindChildren(classes, "iclass")) {
    auto* iclass_proto = result.Add();
    iclass_proto->set_id(ReadAttribute(iclass, "id"));
    iclass_proto->set_name(ReadAttribute(iclass, "name"));

    const auto docvars = FindChild(iclass, "docvars");
    if (!docvars.ok()) return docvars.status();
    const auto docvars_proto = ParseDocVars(docvars.ValueOrDie());
    if (!docvars_proto.ok()) return docvars_proto.status();
    *iclass_proto->mutable_docvars() = docvars_proto.ValueOrDie();

    const auto encodings = ParseInstructionEncodings(iclass);
    if (!encodings.ok()) return encodings.status();
    *iclass_proto->mutable_encodings() = encodings.ValueOrDie();
  }
  return result;
}

// Symbol labels may appear {enclosed} in asm templates to denote optionality.
// This methods returns their canonical representation to allow comparing them.
absl::string_view GetCanonicalLabel(absl::string_view label) {
  return absl::StartsWith(label, "{") && absl::EndsWith(label, "}")
             ? label.substr(1, label.size() - 2)
             : label;
}

// Parses operand definitions from the given <explanations> XML node.
Status ParseExplanations(XMLNode* explanations, XmlInstruction* instruction) {
  CHECK(explanations != nullptr);
  CHECK(instruction != nullptr);

  for (XMLElement* const expl : FindChildren(explanations, "explanation")) {
    // Explanations may target only a subset of extracted instruction encodings.
    const absl::flat_hash_set<std::string> affected_encodings = absl::StrSplit(
        ReadAttribute(expl, "enclist"), ", ", absl::SkipWhitespace());

    // Symbols' links & labels allow linking back to asm templates.
    const auto symbol = FindChild(expl, "symbol");
    if (!symbol.ok()) return symbol.status();
    const std::string id = ReadAttribute(symbol.ValueOrDie(), "link");
    const std::string label = ReadSimpleText(symbol.ValueOrDie());

    // Two (exclusive) types of explanations may be encountered: short <account>
    // descriptions and longer <definition> blocks featuring value tables.
    XMLElement* const account = expl->FirstChildElement("account");
    XMLElement* const definition = expl->FirstChildElement("definition");
    if (!account && !definition) {
      return NotFoundError(
          absl::StrCat("Explanation missing:\n", DebugString(expl)));
    } else if (account && definition) {
      return InvalidArgumentError(
          absl::StrCat("<account> and <definition> are mutually exclusive:\n",
                       DebugString(expl)));
    }
    XMLElement* const container = account != nullptr ? account : definition;

    // Spec may contain multiple fields and even subfields, e.g: "op1:CRm<0>".
    const std::string encoded_in = ReadAttribute(container, "encodedin");

    // Parse HTML explanation blocks into Markdown.
    bool intro = true;
    std::string explanation;
    for (const auto* const block : FindChildren(container, nullptr /* all */)) {
      if (!intro) absl::StrAppend(&explanation, "\n\n");
      absl::StrAppend(&explanation, ExportToMarkdown(block));
      // Contrary to <account> explanations <definition> blocks never explicitly
      // mention the `encoded_in` reference in their <intro> thus we append it.
      if (intro && definition) {
        absl::StrAppend(&explanation, " encoded in `", encoded_in, "`");
      }
      intro = false;
    }

    // Apply reconstructed explanations to their related asm template operands.
    for (auto& instruction_class : *instruction->mutable_classes()) {
      for (auto& encoding : *instruction_class.mutable_encodings()) {
        if (!affected_encodings.contains(encoding.id())) continue;
        for (auto& piece : *encoding.mutable_asm_template()->mutable_pieces()) {
          if (!piece.has_symbol()) continue;
          auto* const symbol = piece.mutable_symbol();
          if (symbol->id() != id) continue;
          if (GetCanonicalLabel(symbol->label()) != GetCanonicalLabel(label)) {
            return FailedPreconditionError(absl::StrCat(
                "Expected label '", symbol->label(), "', found '", label,
                "' for symbol '", id, "' in\n:", DebugString(expl)));
          }
          symbol->set_encoded_in(encoded_in);
          symbol->set_explanation(explanation);
        }
      }
    }
  }
  return OkStatus();
}

}  // namespace

StatusOr<XmlIndex> ParseXmlIndex(const std::string& filename) {
  XmlIndex index;

  XMLDocument xml_doc;
  const Status load_status = GetStatus(xml_doc.LoadFile(filename.c_str()));
  if (!load_status.ok()) return load_status;

  const auto root = FindChild(&xml_doc, "alphaindex");
  if (!root.ok()) return root.status();

  const auto toptitle = FindChild(root.ValueOrDie(), "toptitle");
  if (!toptitle.ok()) return toptitle.status();
  const std::string isa =
      ReadAttribute(toptitle.ValueOrDie(), "instructionset");
  if (isa == "A32") {
    index.set_isa(Isa::A32);
  } else if (isa == "A64") {
    index.set_isa(Isa::A64);
  } else {
    return FailedPreconditionError(absl::StrCat("Unsupported ISA '", isa, "'"));
  }

  const auto iforms = FindChild(root.ValueOrDie(), "iforms");
  if (!iforms.ok()) return iforms.status();
  for (XMLElement* iform : FindChildren(iforms.ValueOrDie(), "iform")) {
    XmlIndex::File* file = index.add_files();
    file->set_filename(ReadAttribute(iform, "iformfile"));
    file->set_heading(ReadAttribute(iform, "heading"));
    file->set_xml_id(ReadAttribute(iform, "id"));
    file->set_description(ExportToMarkdown(iform));
  }

  return index;
}

StatusOr<XmlInstruction> ParseXmlInstruction(const std::string& filename) {
  XmlInstruction instruction;

  XMLDocument xml_doc;
  const Status load_status = GetStatus(xml_doc.LoadFile(filename.c_str()));
  if (!load_status.ok()) return load_status;

  const auto root = FindChild(&xml_doc, "instructionsection");
  if (!root.ok()) return root.status();

  instruction.set_xml_id(ReadAttribute(root.ValueOrDie(), "id"));
  const auto heading = FindChild(root.ValueOrDie(), "heading");
  if (!heading.ok()) return heading.status();
  instruction.set_heading(ReadSimpleText(heading.ValueOrDie()));
  const auto docvars = FindChild(root.ValueOrDie(), "docvars");
  if (!docvars.ok()) return docvars.status();
  const auto docvars_proto = ParseDocVars(docvars.ValueOrDie());
  if (!docvars_proto.ok()) return docvars_proto.status();
  *instruction.mutable_docvars() = docvars_proto.ValueOrDie();

  const auto desc = FindChild(root.ValueOrDie(), "desc");
  if (!desc.ok()) return desc.status();
  const Status desc_status = ParseDescriptions(desc.ValueOrDie(), &instruction);
  if (!desc_status.ok()) return desc_status;

  const auto classes = FindChild(root.ValueOrDie(), "classes");
  if (!classes.ok()) return classes.status();
  const auto classes_proto = ParseInstructionClasses(classes.ValueOrDie());
  if (!classes_proto.ok()) return classes_proto.status();
  *instruction.mutable_classes() = classes_proto.ValueOrDie();

  auto* const expl = root.ValueOrDie()->FirstChildElement("explanations");
  if (expl != nullptr) {  // Explanations are optional.
    const Status expl_status = ParseExplanations(expl, &instruction);
    if (!expl_status.ok()) return expl_status;
  }

  return instruction;
}

StatusOr<XmlDatabase> ParseXmlDatabase(const std::string& path) {
  XmlDatabase database;

  const auto base_index = ParseXmlIndex(file::JoinPath(path, "index.xml"));
  if (!base_index.ok()) return base_index.status();
  *database.mutable_base_index() = base_index.ValueOrDie();

  const auto fp_simd_index =
      ParseXmlIndex(file::JoinPath(path, "fpsimdindex.xml"));
  if (!fp_simd_index.ok()) return fp_simd_index.status();
  *database.mutable_fp_simd_index() = fp_simd_index.ValueOrDie();

  for (const auto& index : {database.base_index(), database.fp_simd_index()}) {
    for (const auto& file : index.files()) {
      const std::string instruction_filename =
          file::JoinPath(path, file.filename());
      const auto instruction = ParseXmlInstruction(instruction_filename);
      if (!instruction.ok()) {
        return Annotate(
            instruction.status(),
            absl::StrCat("while processing file:\n", file.DebugString()));
      }
      *database.add_instructions() = instruction.ValueOrDie();
    }
  }

  return database;
}

XmlDatabase ParseXmlDatabaseOrDie(const std::string& path) {
  return ParseXmlDatabase(path).ValueOrDie();
}

}  // namespace xml
}  // namespace arm
}  // namespace exegesis
