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

#ifndef EXEGESIS_LLVM_ASSEMBLER_DISASSEMBLER_H_
#define EXEGESIS_LLVM_ASSEMBLER_DISASSEMBLER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "exegesis/llvm/assembler_disassembler.pb.h"
#include "exegesis/llvm/disassembler.h"
#include "exegesis/llvm/inline_asm.h"
#include "exegesis/proto/instructions.pb.h"
#include "llvm/IR/InlineAsm.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::StatusOr;

// An assembler-disassembler, which enables one to parse a line of assembly
// code, and get the output from the LLVM disassembler in both the Intel and
// AT&T syntaxes.
// The main usage for this is to normalize the input for the inline assembler,
// so as to remove the ambiguities as much as possible.
class AssemblerDisassembler {
 public:
  AssemblerDisassembler();

  // Assembles (but does not execute) the assembly code given in code, and fill
  // the fields in result.
  StatusOr<AssemblerDisassemblerResult> AssembleDisassemble(
      const std::string& code, llvm::InlineAsm::AsmDialect asm_dialect);

  // Disassembles the binary code given in encoded_instruction
  // It is assumed that the memory pointed to by binary_code either holds the
  // whole instruction or at least is long enough so that the LLVM disassembler
  // stops within the bounds of the allocated memory.
  StatusOr<AssemblerDisassemblerResult> Disassemble(
      const std::vector<uint8_t>& encoded_instruction);

  // Interprets the given input depending on 'interpretation'. The second
  // element of the result contains the interpretation that was used.
  std::pair<StatusOr<AssemblerDisassemblerResult>,
            AssemblerDisassemblerInterpretation>
  AssembleDisassemble(const std::string& input,
                      AssemblerDisassemblerInterpretation interpretation);

 private:
  // The jit compiler, that is initialized once and re-used when
  // AssembleDisassemble is used.
  std::unique_ptr<JitCompiler> jit_;

  // The disassembler, that is initialized once and re-used when
  // AssembleDisassemble is used.
  std::unique_ptr<Disassembler> disasm_;
};

}  // namespace exegesis

#endif  // EXEGESIS_LLVM_ASSEMBLER_DISASSEMBLER_H_
