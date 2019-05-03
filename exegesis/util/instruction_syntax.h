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

#ifndef EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_
#define EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_

#include <string>

#include "absl/strings/string_view.h"
#include "exegesis/proto/instructions.pb.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {

// Parses a code string in assembly format and returns a corresponding
// InstructionFormat.
// NOTE(bdb): This only handles x86 prefixes.
// TODO(bdb): Make this x86-independent.
InstructionFormat ParseAssemblyStringOrDie(const std::string& code);

// Returns an assembler-ready string corresponding to the InstructionFormat
// passed as argument.
std::string ConvertToCodeString(const InstructionFormat& instruction);

// NOTE(user): These functions were added in preparation for the support
// for multiple syntaxes. The comments are already written for the case with
// multiple syntaxes, even though the implementation only deals with the
// single-syntax case. We will make the switch as soon as we turn
// InstructionProto.vendor_syntax into a repeated field.

// Returns a unique mutable vendor syntax for the given instruction:
// 1. If the vendor_syntax field is empty, adds a new value to it and returns
//    this new value.
// 2. If there is a single value in vendor_syntax, returns this value.
// 3. If there is more than one value, causes a CHECK failure.
InstructionFormat* GetOrAddUniqueVendorSyntaxOrDie(
    InstructionProto* instruction);

// Returns a vendor syntax of the instruction. This version should be used in
// call sites where the selected vendor syntax does not matter.
//
// Causes a CHECK failure if the instruction has no vendor syntax.
inline const InstructionFormat& GetAnyVendorSyntaxOrDie(
    const InstructionProto& instruction) {
  CHECK_GT(instruction.vendor_syntax_size(), 0);
  return instruction.vendor_syntax(0);
}

// Returns the vendor syntax of the instruction. This version should be used
// when it is expected that there is exactly one vendor syntax.
//
// Causes a CHECK failure if the instruction has no vendor syntax, or if it has
// more than one.
inline const InstructionFormat& GetUniqueVendorSyntaxOrDie(
    const InstructionProto& instruction) {
  CHECK_EQ(instruction.vendor_syntax_size(), 1);
  return instruction.vendor_syntax(0);
}

// Returns the vendor syntax that has the highest number of operands. Note that
// all operands that are encoded in the binary encoding of the instruction
// should be present in all syntaxes, but some syntaxes may have additional,
// implicitly-encoded operands. A notable example are string instructions (e.g.
// STOS), which have two equivalent versions:
//   - no operand version, e.g. STOSB
//   - a version with explicit operands, e.g. STOS BYTE PTR [RDI].
//
// Causes a CHECK failure if the instruction has no vendor syntax.
const InstructionFormat& GetVendorSyntaxWithMostOperandsOrDie(
    const InstructionProto& instruction);

// Returns true if 'mnemonic' is the mnemonic of one or more vendor syntaxes of
// 'instruction'. Otherwise, returns false.
bool HasMnemonicInVendorSyntax(const InstructionProto& instruction,
                               absl::string_view mnemonic);

// Returns true if 'mnemonic_set' contains a mnemonic of one or more vendor
// syntaxes of 'instruction'. Returns false otherwise.
template <typename SetType>
bool ContainsVendorSyntaxMnemonic(const SetType& mnemonic_set,
                                  const InstructionProto& instruction) {
  for (const InstructionFormat& vendor_syntax : instruction.vendor_syntax()) {
    if (gtl::ContainsKey(mnemonic_set, vendor_syntax.mnemonic())) {
      return true;
    }
  }
  return false;
}

// Returns a value from 'collection' for a mnemonic of a vendor syntax of
// 'instruction'. If multiple mnemonics of the instruction are present in
// 'collection', returns the value for the first one; if no mnemonic of the
// instruction is present, returns nullptr.
// Note that when the instruction does not have any vendor syntax, this function
// returns nullptr.
template <typename MapType>
const typename MapType::value_type::second_type*
FindByVendorSyntaxMnemonicOrNull(const MapType& collection,
                                 const InstructionProto& instruction) {
  for (const InstructionFormat& vendor_syntax : instruction.vendor_syntax()) {
    const auto* const value =
        gtl::FindOrNull(collection, vendor_syntax.mnemonic());
    if (value != nullptr) return value;
  }
  return nullptr;
}

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_
