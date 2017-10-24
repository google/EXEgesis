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

#include "exegesis/llvm/assembler_disassembler.h"

#include <utility>

#include "absl/strings/str_cat.h"
#include "exegesis/llvm/assembler_disassembler.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/strings.h"
#include "glog/logging.h"
#include "llvm/IR/InlineAsm.h"
#include "util/task/canonical_errors.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace {

using ::exegesis::util::InternalError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::StatusOr;

// We're never actually running the assembled code on the host, so we use the
// most complete CPU in terms of features so that we can assemble all
// instructions.
constexpr const char kMcpu[] = "cannonlake";

}  // namespace

AssemblerDisassembler::AssemblerDisassembler()
    : jit_(new JitCompiler(kMcpu)), disasm_(new Disassembler("")) {}

StatusOr<AssemblerDisassemblerResult>
AssemblerDisassembler::AssembleDisassemble(
    const std::string& code, llvm::InlineAsm::AsmDialect asm_dialect) {
  const auto function = jit_->CompileInlineAssemblyToFunction(
      1, "\t" + code,
      /*loop_constraints=*/"", asm_dialect);
  if (!function.ok()) {
    return InvalidArgumentError(
        absl::StrCat("Could not assemble '", code,
                     "': ", ToStringView(function.status().error_message())));
  }
  if (function.ValueOrDie().size <= 0) {
    return InvalidArgumentError(
        absl::StrCat("Non-positive size for '", code, "'"));
  }
  const auto data = reinterpret_cast<const uint8_t*>(function.ValueOrDie().ptr);
  const std::vector<uint8_t> encoded_instruction(
      data, data + function.ValueOrDie().size);
  return Disassemble(encoded_instruction);
}

std::pair<StatusOr<AssemblerDisassemblerResult>,
          AssemblerDisassemblerInterpretation>
AssemblerDisassembler::AssembleDisassemble(
    const std::string& input,
    AssemblerDisassemblerInterpretation interpretation) {
  switch (interpretation) {
    case AssemblerDisassemblerInterpretation::
        HUMAN_READABLE_BINARY_OR_INTEL_ASM: {
      const auto bytes_or_status = ParseHexString(input);
      if (bytes_or_status.ok()) {
        return std::make_pair(Disassemble(bytes_or_status.ValueOrDie()),
                              HUMAN_READABLE_BINARY);
      }
      return std::make_pair(
          AssembleDisassemble(input, llvm::InlineAsm::AD_Intel),
          AssemblerDisassemblerInterpretation::INTEL_ASM);
    }
    case AssemblerDisassemblerInterpretation::INTEL_ASM:
      return std::make_pair(
          AssembleDisassemble(input, llvm::InlineAsm::AD_Intel),
          AssemblerDisassemblerInterpretation::INTEL_ASM);
    case AssemblerDisassemblerInterpretation::ATT_ASM:
      return std::make_pair(AssembleDisassemble(input, llvm::InlineAsm::AD_ATT),
                            AssemblerDisassemblerInterpretation::ATT_ASM);
    case AssemblerDisassemblerInterpretation::HUMAN_READABLE_BINARY: {
      const auto bytes_or_status = ParseHexString(input);
      if (!bytes_or_status.ok()) {
        return std::make_pair(
            InvalidArgumentError(absl::StrCat(
                "Input '", input, "' is not in human readable binary format")),
            AssemblerDisassemblerInterpretation::HUMAN_READABLE_BINARY);
      }
      return std::make_pair(
          Disassemble(bytes_or_status.ValueOrDie()),
          AssemblerDisassemblerInterpretation::HUMAN_READABLE_BINARY);
    }
    default:
      return std::make_pair(InternalError("unreachable"),
                            AssemblerDisassemblerInterpretation::INTEL_ASM);
  }
}

StatusOr<AssemblerDisassemblerResult> AssemblerDisassembler::Disassemble(
    const std::vector<uint8_t>& encoded_instruction) {
  AssemblerDisassemblerResult result;
  std::vector<std::string> llvm_operands;
  std::string intel_code;
  std::string att_code;
  unsigned llvm_opcode = 0;
  const int binary_encoding_size_in_bytes = disasm_->Disassemble(
      encoded_instruction, &llvm_opcode, result.mutable_llvm_mnemonic(),
      &llvm_operands, &intel_code, &att_code);
  if (binary_encoding_size_in_bytes == 0) {
    return InvalidArgumentError(
        absl::StrCat("Could not disassemble: ",
                     ToHumanReadableHexString(encoded_instruction)));
  }
  CHECK_LE(binary_encoding_size_in_bytes, encoded_instruction.size());
  // Make a copy of the binary encoding of the instruction.
  result.mutable_binary_encoding()->assign(
      encoded_instruction.begin(),
      encoded_instruction.begin() + binary_encoding_size_in_bytes);
  *result.mutable_intel_syntax() = ParseAssemblyStringOrDie(intel_code);
  *result.mutable_att_syntax() = ParseAssemblyStringOrDie(att_code);
  result.set_llvm_opcode(llvm_opcode);
  return result;
}

}  // namespace exegesis
