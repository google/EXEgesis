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

#ifndef EXEGESIS_LLVM_DISASM_H_
#define EXEGESIS_LLVM_DISASM_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"

namespace exegesis {

// A wrapper around the LLVM disassembler.
class Disassembler {
 public:
  // Creates a disassembler for triple_name. If triple_name is "", the
  // default one will be used.
  explicit Disassembler(const std::string& triple_name);

  // Disassembles an instruction from a span of bytes. The return value is the
  // length of the disassembled instruction in bytes (or 0 on error). The values
  // pointed to by the other parameters are modified as follows:
  //   *llvm_opcode contains the LLVM opcode of the disassembled instruction,
  //   *llvm_mnemonic contains the LLVM mnemonic of the disassembled
  //   instruction,
  //   *llvm_operands contains the list of LLVM operand in text form.
  //   *intel_instruction contains the x86 instruction (mnemonic and operands)
  //   in Intel format.
  //   *att_instruction contains the x86 instruction (mnemonic and operands)
  //   in ATT format.
  int Disassemble(absl::Span<const uint8_t> bytes, unsigned* llvm_opcode,
                  std::string* llvm_mnemonic,
                  std::vector<std::string>* llvm_operands,
                  std::string* intel_instruction,
                  std::string* att_instruction) const;

  // Disassembles a hex string and returns a (possibly multi-line) string
  // containing: "Address; Hex code; Intel syntax; ATT syntax; LLVM Mnemonic"
  // For example: disasm.DisassembleString("48B85634129078563412") returns:
  // "00000000; 48B85634129078563412; movabs rax, 0x1234567890123456; movabsq "
  // "$0x1234567890123456, %rax; MOV64ri",
  std::string DisassembleHexString(const std::string& hex_bytes) const;

 private:
  // Initializes the Disassembler. This is done automatically when constructed.
  void Init();

  // Disassembles an intruction from a span of bytes. The returns the length of
  // the disassembled instruction in bytes and an MCInst (or nullopt on error).
  absl::optional<std::pair<int, llvm::MCInst>> DisassembleToMCInst(
      absl::Span<const uint8_t> bytes) const;

  // Target arch to assemble for.
  std::string arch_name_;

  // Target triple to assemble for.
  std::string triple_name_;

  // LLVM triple looked up from triple_name_.
  llvm::Triple triple_;

  // The LLVM which we are using.
  const llvm::Target* target_;

  std::unique_ptr<const llvm::MCRegisterInfo> register_info_;

  std::unique_ptr<const llvm::MCAsmInfo> asm_info_;

  std::unique_ptr<llvm::MCObjectFileInfo> object_file_info_;

  std::unique_ptr<const llvm::SourceMgr> source_manager_;

  std::unique_ptr<llvm::MCContext> mc_context_;

  std::unique_ptr<const llvm::MCDisassembler> disasm_;

  std::unique_ptr<const llvm::MCInstrInfo> instruction_info_;

  std::unique_ptr<const llvm::MCSubtargetInfo> sub_target_info_;

  const llvm::MCCodeEmitter* code_emitter_;

  const llvm::MCAsmBackend* asm_backend_;

  std::unique_ptr<llvm::MCInstPrinter> intel_instruction_printer_;

  std::unique_ptr<llvm::MCInstPrinter> att_instruction_printer_;

  // Print marked up assembly.
  const bool use_markup_ = false;

  // Print immediate operands in hex.
  const bool print_imm_hex_ = true;

  // Hide instruction encodings.
  const bool hide_encoding_ = true;
};

}  // namespace exegesis

#endif  // EXEGESIS_LLVM_DISASM_H_
