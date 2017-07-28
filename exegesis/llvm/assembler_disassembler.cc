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

#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/strings.h"
#include "glog/logging.h"
#include "llvm/IR/InlineAsm.h"
#include "strings/str_cat.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace {

using ::exegesis::util::StatusOr;

// We're never actually running the assembled code on the host, so we use the
// most complete CPU in terms of features so that we can assemble all
// instructions.
constexpr const char kMcpu[] = "cannonlake";

}  // namespace

AssemblerDisassembler::AssemblerDisassembler() {
  jit_.reset(new JitCompiler(kMcpu, JitCompiler::RETURN_NULLPTR_ON_ERROR));
  disasm_.reset(new Disassembler(""));
}

void AssemblerDisassembler::Reset() {
  llvm_opcode_ = 0;
  llvm_mnemonic_.clear();
  binary_encoding_.clear();
  intel_format_.Clear();
  att_format_.Clear();
}

bool AssemblerDisassembler::AssembleDisassemble(
    const string& code, llvm::InlineAsm::AsmDialect asm_dialect) {
  Reset();
  VoidFunction function = jit_->CompileInlineAssemblyToFunction(
      1, "\t" + code,
      /*loop_constraints=*/"", asm_dialect);
  if (!function.IsValid()) {
    LOG(INFO) << "Error on: " << code;
    return false;
  }
  const uint8_t* const encoded_instruction =
      reinterpret_cast<const uint8_t*>(function.ptr);
  return Disassemble(encoded_instruction);
}

bool AssemblerDisassembler::AssembleDisassemble(const string& code) {
  return AssembleDisassemble(code, llvm::InlineAsm::AD_Intel);
}

bool AssemblerDisassembler::Disassemble(const uint8_t* encoded_instruction) {
  Reset();
  std::string llvm_mnemonic;
  std::vector<std::string> llvm_operands;
  std::string intel_code;
  std::string att_code;
  const int binary_encoding_size_in_bytes =
      disasm_->Disassemble(encoded_instruction, &llvm_opcode_, &llvm_mnemonic,
                           &llvm_operands, &intel_code, &att_code);
  if (binary_encoding_size_in_bytes == 0) {
    LOG(INFO) << "could not disassemble";
    return false;
  }
  // Make a copy of the binary encoding of the instruction.
  binary_encoding_.assign(encoded_instruction,
                          encoded_instruction + binary_encoding_size_in_bytes);
  llvm_mnemonic_ = llvm_mnemonic;
  intel_format_ = ParseAssemblyStringOrDie(intel_code);
  att_format_ = ParseAssemblyStringOrDie(att_code);
  return IsValid();
}

string AssemblerDisassembler::GetHumanReadableBinaryEncoding() const {
  return ToHumanReadableHexString(binary_encoding_);
}

string AssemblerDisassembler::GetPastableBinaryEncoding() const {
  return ToPastableHexString(binary_encoding_);
}

string AssemblerDisassembler::DebugString() const {
  string buffer;
  StrAppend(&buffer, "LLVM mnemonic: ", llvm_mnemonic_, "\n");
  StrAppend(&buffer, "Intel: ", ConvertToCodeString(intel_format_), "\n");
  StrAppend(&buffer, "AT&T: ", ConvertToCodeString(att_format_), "\n");
  StrAppend(&buffer, GetHumanReadableBinaryEncoding());
  return buffer;
}

std::vector<uint8_t> ParseBinaryInstructionAndPadWithNops(
    StringPiece input_line) {
  std::vector<uint8_t> bytes;
  StatusOr<std::vector<uint8_t>> bytes_or_status =
      ParseHexString(string(input_line));
  if (!bytes_or_status.ok()) {
    return bytes;
  }

  // If the vector is non-empty it means that the user entered some binary
  // code, and we have things to disassemble. Add 32 NOP bytes at the end of
  // the vector to make sure that the disassembler always has something to
  // stop at (even if the bytes that the user entered are only prefixes).
  constexpr int kPaddingSize = 32;
  bytes = std::move(bytes_or_status.ValueOrDie());
  bytes.insert(bytes.end(), kPaddingSize, 0x90);
  return bytes;
}

}  // namespace exegesis
