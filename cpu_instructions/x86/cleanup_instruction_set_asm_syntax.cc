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

#include "cpu_instructions/x86/cleanup_instruction_set_asm_syntax.h"

#include "strings/string.h"
#include <unordered_set>

#include "glog/logging.h"
#include "strings/str_cat.h"
#include "strings/string_view.h"
#include "util/gtl/map_util.h"
#include "cpu_instructions/base/cleanup_instruction_set.h"
#include "cpu_instructions/proto/instructions.pb.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {
namespace {

using ::cpu_instructions::util::InvalidArgumentError;
using ::cpu_instructions::util::Status;

char GetSuffixFromPointerType(StringPiece operand) {
  string result;
  constexpr const char* const kPointerTypes[] = {"BYTE", "WORD", "DWORD",
                                                 "QWORD"};
  for (const char* const pointer_type : kPointerTypes) {
    if (operand.starts_with(pointer_type)) {
      return pointer_type[0];
    }
  }
  return '\0';
}

}  // anonymous namespace

Status AddIntelAsmSyntax(InstructionSetProto* instruction_set) {
  const std::unordered_set<string> kStringMnemonics = {
      "CMPS", "INS", "LODS", "MOVS", "OUTS", "SCAS", "STOS"};
  Status status = Status::OK;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    InstructionFormat* const syntax = instruction.mutable_syntax();
    *syntax = instruction.vendor_syntax();
    if (ContainsKey(kStringMnemonics, syntax->mnemonic())) {
      // Adds a suffix to all the string mnemonics, because the LLVM assembler
      // does not recognize the mnemonics without the suffix.
      if (syntax->operands().empty()) {
        status = InvalidArgumentError(StrCat(
            "Unexpected number of arguments:\n", instruction.DebugString()));
        LOG(ERROR) << status;
        continue;
      }
      char suffix = GetSuffixFromPointerType(syntax->operands(0).name());
      if (suffix == '\0' && syntax->operands().size() > 1) {
        suffix = GetSuffixFromPointerType(syntax->operands(1).name());
      }
      syntax->set_mnemonic(syntax->mnemonic() + suffix);
    } else if (syntax->mnemonic() == "INVLPG") {
      // The assembler only understand m8, and not general memory references.
      syntax->mutable_operands(0)->set_name("m8");
    } else if (syntax->mnemonic() == "MOV" &&
               syntax->operands(1).name() == "imm64") {
      // MOV r/m64, imm64" uses the mnemonic MOVABS in LLVM.
      syntax->set_mnemonic("MOVABS");
    } else if (syntax->mnemonic() == "LSL" &&
               syntax->operands(0).name() == "r64") {
      // Replace r32/m16 with r64. This is a simplification.
      syntax->mutable_operands(1)->set_name("r64");
    } else if (syntax->mnemonic() == "NOP" && syntax->operands().size() == 1) {
      // Consider only NOP m32. This is a simplification.
      syntax->mutable_operands(0)->set_name("m32");
    } else if (syntax->mnemonic() == "MOVSD" && syntax->operands().empty()) {
      // Disambiguate MOVSD with argument.
      auto* operand = syntax->add_operands();
      operand->set_name("DWORD PTR [RDI]");
      operand->set_usage(InstructionOperand::USAGE_WRITE);
      operand = syntax->add_operands();
      operand->set_name("DWORD PTR [RSI]");
      operand->set_usage(InstructionOperand::USAGE_READ);
    }
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddIntelAsmSyntax, kNotInDefaultPipeline);

}  // namespace x86
}  // namespace cpu_instructions
