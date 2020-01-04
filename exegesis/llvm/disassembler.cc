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

#include "exegesis/llvm/disassembler.h"

#include <algorithm>
#include <string>
#include <utility>

#include "absl/base/macros.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_format.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "exegesis/llvm/llvm_utils.h"
#include "glog/logging.h"
#include "llvm/MC/MCAsmInfo.h"
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

Disassembler::Disassembler(const std::string& triple_name)
    : triple_name_(triple_name.c_str()) {
  Init();
}

void Disassembler::Init() {
  EnsureLLVMWasInitialized();

  if (triple_name_.empty()) {
    // Figure out the target triple.
    triple_name_ = llvm::sys::getDefaultTargetTriple();
  }
  triple_.setTriple(llvm::Triple::normalize(triple_name_.c_str()));

  // Get the target specific parser.
  std::string error_string;
  target_ =
      llvm::TargetRegistry::lookupTarget(arch_name_, triple_, error_string);
  CHECK(nullptr != target_) << "Could not detect the target.";
  CHECK(error_string.empty()) << error_string;

  // Create register info.
  register_info_.reset(target_->createMCRegInfo(triple_name_));
  CHECK(nullptr != register_info_) << "Unable to create target register info.";

  // Create assembler info.
  asm_info_.reset(target_->createMCAsmInfo(*register_info_, triple_name_,
                                           llvm::MCTargetOptions()));
  CHECK(nullptr != asm_info_) << "Unable to create target asm info.";

  // FIXME: This is not pretty. MCContext has a ptr to MCObjectFileInfo and
  // MCObjectFileInfo needs a MCContext reference in order to initialize itself.
  object_file_info_ = absl::make_unique<llvm::MCObjectFileInfo>();
  bool is_pic = false;

  // Create an empty SourceMgr.
  source_manager_ = absl::make_unique<llvm::SourceMgr>();

  mc_context_ = absl::make_unique<llvm::MCContext>(
      asm_info_.get(), register_info_.get(), object_file_info_.get(),
      source_manager_.get());
  object_file_info_->InitMCObjectFileInfo(llvm::Triple(triple_name_), is_pic,
                                          *mc_context_);

  CHECK(error_string.empty()) << error_string;

  instruction_info_.reset(target_->createMCInstrInfo());
  sub_target_info_.reset(target_->createMCSubtargetInfo(triple_name_, "", ""));

  code_emitter_ = hide_encoding_
                      ? nullptr
                      : target_->createMCCodeEmitter(
                            *instruction_info_, *register_info_, *mc_context_);
  llvm::MCTargetOptions options;
  asm_backend_ = hide_encoding_
                     ? nullptr
                     : target_->createMCAsmBackend(*sub_target_info_,
                                                   *register_info_, options);

  intel_instruction_printer_.reset(target_->createMCInstPrinter(
      triple_, 1, *asm_info_, *instruction_info_, *register_info_));
  intel_instruction_printer_->setUseMarkup(use_markup_);
  intel_instruction_printer_->setPrintImmHex(print_imm_hex_);

  att_instruction_printer_.reset(target_->createMCInstPrinter(
      triple_, 0, *asm_info_, *instruction_info_, *register_info_));
  att_instruction_printer_->setUseMarkup(use_markup_);
  att_instruction_printer_->setPrintImmHex(print_imm_hex_);

  disasm_.reset(target_->createMCDisassembler(*sub_target_info_, *mc_context_));
}

absl::optional<std::pair<int, llvm::MCInst>> Disassembler::DisassembleToMCInst(
    absl::Span<const uint8_t> bytes) const {
  llvm::MCInst inst;
  uint64_t decode_size;
  const llvm::MCDisassembler::DecodeStatus decode_status =
      disasm_->getInstruction(
          inst, decode_size,
          llvm::ArrayRef<uint8_t>{bytes.data(), bytes.size()}, 0, llvm::nulls(),
          llvm::nulls());
  if (decode_status == llvm::MCDisassembler::Fail) return absl::nullopt;
  if (decode_status == llvm::MCDisassembler::SoftFail) {
    LOG(WARNING) << "Potentially undefined instruction encoding";
  }
  return std::make_pair(static_cast<int>(decode_size), inst);
}

int Disassembler::Disassemble(absl::Span<const uint8_t> bytes,
                              unsigned* const llvm_opcode,
                              std::string* const llvm_mnemonic,
                              std::vector<std::string>* const llvm_operands,
                              std::string* const intel_instruction,
                              std::string* const att_instruction) const {
  *llvm_opcode = 0;
  llvm_mnemonic->clear();
  llvm_operands->clear();
  intel_instruction->clear();
  att_instruction->clear();

  const auto result = DisassembleToMCInst(bytes);
  if (!result) return 0;
  const int decode_size = result->first;
  const llvm::MCInst& instruction = result->second;

  std::string tmp;
  tmp.clear();
  llvm::raw_string_ostream att_stream(tmp);
  att_instruction_printer_->printInst(&instruction, att_stream, "",
                                      *sub_target_info_);
  att_stream.flush();
  att_instruction->assign(tmp);

  tmp.clear();
  llvm::raw_string_ostream intel_stream(tmp);
  intel_instruction_printer_->printInst(&instruction, intel_stream, "",
                                        *sub_target_info_);
  intel_stream.flush();
  intel_instruction->assign(tmp);

  *llvm_opcode = instruction.getOpcode();
  *llvm_mnemonic = instruction_info_->getName(instruction.getOpcode());
  llvm_operands->resize(instruction.getNumOperands());
  for (int i = 0; i < instruction.getNumOperands(); ++i) {
    tmp.clear();
    llvm::raw_string_ostream operand_stream(tmp);
    const llvm::MCOperand operand = instruction.getOperand(i);
    operand_stream << operand;
    operand_stream.flush();
    (*llvm_operands)[i].assign(tmp);
  }
  return decode_size;
}

namespace {
uint8_t HexValue(char c) {
  uint8_t result = -1;
  if (c >= 'a' && c <= 'f') {
    result = c - 'a' + 10;
  } else if (c >= 'A' && c <= 'F') {
    result = c - 'A' + 10;
  } else if (c >= '0' && c <= '9') {
    result = c - '0';
  } else {
    LOG(FATAL) << "Illegal hex character: " << c;
  }
  CHECK_LE(0, result);
  CHECK_GE(15, result);
  return result;
}
}  // namespace

std::string Disassembler::DisassembleHexString(
    const std::string& hex_bytes) const {
  std::string result;
  const int size = hex_bytes.size() / 2;
  CHECK_EQ(0, hex_bytes.size() % 2);
  unsigned llvm_opcode;
  std::string llvm_mnemonic;
  std::vector<std::string> llvm_operands;
  std::string intel_instruction;
  std::string att_instruction;
  uint64_t instruction_size = size;
  constexpr int kMaxX86InstructionSize = 15;
  std::vector<uint8_t> buffer(kMaxX86InstructionSize);
  for (int offset = 0; offset < size; offset += instruction_size) {
    if (offset != 0) result += "\n";
    const int remaining_bytes = std::min(size - offset, kMaxX86InstructionSize);
    for (int i = 0; i < remaining_bytes; ++i) {
      const int pos = 2 * (offset + i);
      buffer[i] = HexValue(hex_bytes[pos]) * 16 + HexValue(hex_bytes[pos + 1]);
    }
    instruction_size =
        Disassemble(buffer, &llvm_opcode, &llvm_mnemonic, &llvm_operands,
                    &intel_instruction, &att_instruction);
    std::replace(intel_instruction.begin(), intel_instruction.end(), '\t', ' ');
    std::replace(att_instruction.begin(), att_instruction.end(), '\t', ' ');
    absl::StrAppendFormat(&result, "%08x; %s;%s;%s; %s", offset,
                          hex_bytes.substr(2 * offset, 2 * instruction_size),
                          intel_instruction, att_instruction, llvm_mnemonic);
  }
  return result;
}
}  // namespace exegesis
