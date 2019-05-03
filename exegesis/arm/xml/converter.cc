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
#include <string>

#include "absl/strings/str_join.h"
#include "exegesis/arm/xml/docvars.pb.h"
#include "exegesis/arm/xml/parser.pb.h"
#include "exegesis/proto/instruction_encoding.pb.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace arm {
namespace xml {

namespace {

InstructionFormat ConvertAsmTemplate(const std::string& mnemonic,
                                     const AsmTemplate& asm_template) {
  InstructionFormat format;
  format.set_mnemonic(mnemonic);
  for (const auto& piece : asm_template.pieces()) {
    const auto& symbol = piece.symbol();
    if (symbol.id().empty()) continue;
    auto* operand = format.add_operands();
    operand->set_name(symbol.label());
    operand->set_description(symbol.explanation());
    // TODO(npaglieri): Infer data type (Register / Immediate / ...)
    // TODO(npaglieri): Infer usage (Read / Write / ReadWrite)
    // TODO(user): Preserve important non-pure-operand information, e.g.
    //                   extra characters in "CAS <Ws>, <Wt>, [<Xn|SP>{,#0}]".
  }
  return format;
}

std::string FirstSetOrEmpty(const std::vector<std::string>& strings) {
  for (const auto& s : strings) {
    if (!s.empty()) return s;
  }
  return "";
}

}  // namespace

ArchitectureProto ConvertToArchitectureProto(const XmlDatabase& xml_database) {
  ArchitectureProto architecture;
  architecture.set_name("ARMv8");

  InstructionSetProto* isp = architecture.mutable_instruction_set();

  // TODO(npaglieri): Add more details, e.g. the version of the database.
  isp->add_source_infos()->set_source_name("ARM XML Database");

  std::map<std::string, int> known_groups;
  for (const auto& index :
       {xml_database.base_index(), xml_database.fp_simd_index()}) {
    for (const auto& file : index.files()) {
      gtl::InsertOrDie(&known_groups, file.xml_id(),
                       isp->instruction_groups_size());
      auto* group = isp->add_instruction_groups();
      group->set_name(file.heading());
      group->set_short_description(file.description());
    }
  }

  for (const auto& xml_instruction : xml_database.instructions()) {
    // ARM documentation suggests that it's always preferable to use the alias.
    const std::string instruction_mnemonic =
        FirstSetOrEmpty({xml_instruction.docvars().alias_mnemonic(),
                         xml_instruction.docvars().mnemonic()});

    // TODO(npaglieri): Decide whether to use the <aliasto> tag to merge groups.
    const int group_index =
        gtl::FindOrDie(known_groups, xml_instruction.xml_id());
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
        instruction->set_description(
            absl::StrJoin({xml_instruction.brief_description(),
                           instruction_class.name(), encoding.name()},
                          " | "));
        // Use any encoding-specific mnemonic (preferring aliases as above when
        // present), otherwise default to the one defined at instruction level.
        const std::string encoding_mnemonic = FirstSetOrEmpty(
            {encoding.docvars().alias_mnemonic(), encoding.docvars().mnemonic(),
             instruction_mnemonic});
        *GetOrAddUniqueVendorSyntaxOrDie(instruction) =
            ConvertAsmTemplate(encoding_mnemonic, encoding.asm_template());
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
