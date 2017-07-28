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
#include <vector>
#include "strings/string.h"

#include "exegesis/llvm/disassembler.h"
#include "exegesis/llvm/inline_asm.h"
#include "exegesis/proto/instructions.pb.h"
#include "llvm/IR/InlineAsm.h"
#include "strings/string_view.h"

namespace exegesis {

// An assembler-disassembler, which enables one to parse a line of assembly
// code, and get the output from the LLVM disassembler in both the Intel and
// AT&T syntaxes.
// The main usage for this is to normalize the input for the inline assembler,
// so as to remove the ambiguities as much as possible.
class AssemblerDisassembler {
 public:
  AssemblerDisassembler();

  // Resets the contents of the assembler-disassembler.
  void Reset();

  // Assembles (but does not execute) the assembly code given in code.
  // If the assembly could not be done, returns false.
  // Otherwise, disassembles the binary that was produced, and fills in the
  // fields opcode_, llvm_mnemonic_, intel_mnemonic_, intel_operands_,
  // att_mnemonic_, att_operands_.
  bool AssembleDisassemble(const string& code);

  // Assembles (and does not execute) the assembly code given in code, which
  // uses asm_dialect syntax. If the assembly could not be done, returns false.
  // Otherwise, disassembles the binary that was produced, and fills in the
  // fields opcode_, llvm_mnemonic_, intel_mnemonic_, intel_operands_,
  // att_mnemonic_, att_operands_.
  bool AssembleDisassemble(const string& code,
                           llvm::InlineAsm::AsmDialect asm_dialect);

  // Disassembles the binary code given in encoded_instruction, and fills in the
  // fields opcode_, llvm_mnemonic_, intel_mnemonic_, intel_operands_,
  // att_mnemonic_, att_operands_.
  // It is assumed that the memory pointed to by binary_code either holds the
  // whole instruction or at least is long enough so that the LLVM disassembler
  // stops within the bounds of the allocated memory.
  bool Disassemble(const uint8_t* encoded_instruction);

  // Returns true if the data in the class is valid.
  bool IsValid() const { return binary_encoding_size_in_bytes() > 0; }

  // Returns the LLVM opcode of the instruction that was disassembled.
  unsigned llvm_opcode() const { return llvm_opcode_; }

  // Returns the LLVM mnemonic of the instruction that was disassembled.
  string llvm_mnemonic() const { return llvm_mnemonic_; }

  // Returns the format for the instruction that was disassembled using the
  // Intel syntax of LLVM. Note(bdb): the Intel syntax of LLVM is sometimes
  // different from the ones of MASM, nasm or yasm.
  const InstructionFormat& intel_format() const { return intel_format_; }

  // Returns the format for the instruction that was disassembled using the
  // AT&T syntax of LLVM . TODO(bdb): investigate how much this syntax is
  // different from the one of gas.
  const InstructionFormat& att_format() const { return att_format_; }

  // Returns the size of the binary encoding of the instruction that was
  // assembled and disassembled.
  int binary_encoding_size_in_bytes() const { return binary_encoding_.size(); }

  // Returns the binary encoding of the instruction.
  const std::vector<uint8_t>& binary_encoding() const {
    return binary_encoding_;
  }

  // Returns human-readable binary encoding for the instruction.
  string GetHumanReadableBinaryEncoding() const;

  // Return copy-pastable binary encoding for the instruction.
  string GetPastableBinaryEncoding() const;

  // Returns the contents of the assembler-disassembler in a human-readable
  // string form.
  string DebugString() const;

 private:
  // The jit compiler, that is initialized once and re-used when
  // AssembleDisassemble is used.
  std::unique_ptr<JitCompiler> jit_;

  // The disassembler, that is initialized once and re-used when
  // AssembleDisassemble is used.
  std::unique_ptr<Disassembler> disasm_;

  unsigned llvm_opcode_;
  string llvm_mnemonic_;
  InstructionFormat intel_format_;
  InstructionFormat att_format_;
  std::vector<uint8_t> binary_encoding_;
};

// Parses "binary" code from the string. Assumes that the binary is entered as a
// sequence of uppercase hexadecimal digits separated by spaces. Stops at first
// part that does not fit this definition.
std::vector<uint8_t> ParseBinaryInstructionAndPadWithNops(
    StringPiece input_line);

}  // namespace exegesis

#endif  // EXEGESIS_LLVM_ASSEMBLER_DISASSEMBLER_H_
