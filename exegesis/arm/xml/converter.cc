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

#include <map>
#include "strings/string.h"

#include "exegesis/arm/xml/docvars.pb.h"
#include "exegesis/arm/xml/parser.pb.h"
#include "exegesis/proto/instruction_encoding.pb.h"
#include "exegesis/proto/instructions.pb.h"
#include "strings/str_join.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace arm {
namespace xml {

namespace {

InstructionFormat ConvertAsmTemplate(const string& mnemonic,
                                     const AsmTemplate& asm_template) {
  InstructionFormat format;
  format.set_mnemonic(mnemonic);
  for (const auto& piece : asm_template.pieces()) {
    const auto& symbol = piece.symbol();
    if (symbol.id().empty()) continue;
    auto* operand = format.add_operands();
    operand->set_name(symbol.label());
    // TODO(npaglieri): Link back to the encoding's instruction_layout fields.
  }
  return format;
}

}  // namespace

ArchitectureProto ConvertToArchitectureProto(const XmlDatabase& xml_database) {
  ArchitectureProto architecture;
  architecture.set_name("ARMv8");

  InstructionSetProto* isp = architecture.mutable_instruction_set();

  // TODO(npaglieri): Add more details, e.g. the version of the database.
  isp->add_source_infos()->set_source_name("ARM XML Database");

  std::map<string, int> known_groups;
  for (const auto& index :
       {xml_database.base_index(), xml_database.fp_simd_index()}) {
    for (const auto& file : index.files()) {
      InsertOrDie(&known_groups, file.xml_id(), isp->instruction_groups_size());
      auto* group = isp->add_instruction_groups();
      group->set_name(file.heading());
      group->set_short_description(file.description());
    }
  }

  for (const auto& xml_instruction : xml_database.instructions()) {
    const string& mnemonic = xml_instruction.docvars().mnemonic();
    const string& alias = xml_instruction.docvars().alias_mnemonic();

    // TODO(npaglieri): Decide whether to use the <aliasto> tag to merge groups.
    const int group_index = FindOrDie(known_groups, xml_instruction.xml_id());
    auto* group = isp->mutable_instruction_groups(group_index);
    group->set_description(xml_instruction.authored_description());
    if (xml_instruction.docvars().cond_setting() == dv::CondSetting::S) {
      group->add_flags_affected()->set_content("S");
    }

    for (const auto& instruction_class : xml_instruction.classes()) {
      for (const auto& encoding : instruction_class.encodings()) {
        auto* instruction = isp->add_instructions();
        instruction->set_instruction_group_index(group_index);
        // The longer authored_description is used as group description.
        instruction->set_description(strings::Join(
            {xml_instruction.brief_description(), instruction_class.name()},
            " | "));
        *instruction->mutable_vendor_syntax() = ConvertAsmTemplate(
            !alias.empty() ? alias : mnemonic, encoding.asm_template());
        // Unfortunately (or not?) ARM is unlikely to provide more features.
        switch (encoding.docvars().feature()) {
          case dv::Feature::CRC:
            instruction->set_feature_name("crc");
            break;
          default:
            break;
        }
        instruction->set_available_in_64_bit(xml_instruction.docvars().isa() ==
                                             dv::Isa::A64);

        instruction->set_binary_encoding_size_bytes(4);
        instruction->set_encoding_scheme(
            encoding.instruction_layout().form_name());
        *instruction->mutable_fixed_size_encoding_specification() =
            encoding.instruction_layout();
      }
    }
  }

  return architecture;
}

}  // namespace xml
}  // namespace arm
}  // namespace exegesis
