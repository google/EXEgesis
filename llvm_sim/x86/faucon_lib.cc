// Copyright 2018 Google Inc.
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

#include "llvm_sim/x86/faucon_lib.h"

#include <iostream>

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/SourceMgr.h"

namespace exegesis {
namespace simulator {

namespace {

// IACA magic markers.
constexpr char kBeginMagicMarker[] = {0x0f, 0x0b, 0xbb, 0x6f, 0x00,
                                      0x00, 0x00, 0x64, 0x67, 0x90};
constexpr char kEndMagicMarker[] = {0xbb, 0xde, 0x00, 0x00, 0x00,
                                    0x64, 0x67, 0x90, 0x0f, 0x0b};

}  // namespace

std::vector<uint8_t> GetIACAMarkedCode(const std::string& FileName) {
  // Open binary and search for marker.
  auto BinaryOrErr = llvm::object::createBinary(FileName);
  if (!BinaryOrErr) {
    std::cerr << "could not open binary\n";
    return {};
  }
  const auto* const Obj =
      llvm::dyn_cast<llvm::object::ObjectFile>(BinaryOrErr.get().getBinary());
  if (!Obj) {
    std::cerr << "not an object file\n";
    return {};
  }
  std::cout << "  format: '" << Obj->getFileFormatName().str() << "'\n";
  std::cout << "  arch: '"
            << llvm::Triple::getArchTypeName(Obj->getArch()).str() << "'\n";

  // Only search code (a.k.a text) sections. Other sections might contains the
  // magic markers for random reasons.
  std::vector<uint8_t> CodeBytes;
  for (const auto& Section : Obj->sections()) {
    if (!Section.isText()) {
      continue;
    }
    llvm::StringRef Code;
    Section.getContents(Code);
    // Find the begin/end markers.
    auto BeginIt = std::search(Code.begin(), Code.end(), kBeginMagicMarker,
                               kBeginMagicMarker + sizeof(kBeginMagicMarker));
    if (BeginIt == Code.end()) {
      continue;
    }
    BeginIt += sizeof(kBeginMagicMarker);
    const auto EndIt = std::search(BeginIt, Code.end(), kEndMagicMarker,
                                   kEndMagicMarker + sizeof(kEndMagicMarker));
    if (EndIt == Code.end()) {
      std::cerr << "found begin marker without end marker\n";
      continue;
    }
    if (BeginIt == EndIt) {
      std::cerr << "ignoring empty code sequence\n";
      continue;
    }
    return std::vector<uint8_t>(BeginIt, EndIt);
  }
  return {};
}

std::vector<llvm::MCInst> ParseMCInsts(const GlobalContext& Context,
                                       llvm::ArrayRef<uint8_t> CodeBytes) {
  std::vector<llvm::MCInst> Result;
  const std::unique_ptr<llvm::MCDisassembler> Disasm(
      Context.Target->createMCDisassembler(*Context.SubtargetInfo,
                                           *Context.LLVMContext));
  llvm::MCInst Inst;
  uint64_t InstSize = 0;
  while (Disasm->getInstruction(Inst, InstSize, CodeBytes, 0, llvm::nulls(),
                                llvm::nulls()) ==
         llvm::MCDisassembler::Success) {
    Result.push_back(Inst);
    CodeBytes = CodeBytes.drop_front(InstSize);
  }
  return Result;
}

std::vector<llvm::MCInst> ParseIACAMarkedCodeFromFile(
    const GlobalContext& Context, const std::string& FileName) {
  const std::vector<uint8_t> CodeBytes = GetIACAMarkedCode(FileName);
  if (CodeBytes.empty()) {
    std::cerr << "cound not find code to analyze\n";
    return {};
  }
  std::cout << "analyzing " << CodeBytes.size() << " bytes\n";

  return ParseMCInsts(Context, CodeBytes);
}

namespace {

// A streamer that puts MCInsts in a vector.
class MCInstStreamer : public llvm::MCStreamer {
 public:
  explicit MCInstStreamer(llvm::MCContext* Context,
                          std::vector<llvm::MCInst>* Result)
      : llvm::MCStreamer(*Context), Result_(Result) {}

  // Implementation of the llvm::MCStreamer interface. We only care about
  // instructions.
  void EmitInstruction(const llvm::MCInst& instruction,
                       const llvm::MCSubtargetInfo& mc_subtarget_info,
                       bool PrintSchedInfo) override {
    Result_->push_back(instruction);
  }

 private:
  // We only care about instructions, we don't implement this part of the API.
  void EmitCommonSymbol(llvm::MCSymbol* symbol, uint64_t size,
                        unsigned byte_alignment) override {}
  bool EmitSymbolAttribute(llvm::MCSymbol* symbol,
                           llvm::MCSymbolAttr attribute) override {
    return false;
  }
  void EmitValueToAlignment(unsigned byte_alignment, int64_t value,
                            unsigned value_size,
                            unsigned max_bytes_to_emit) override {}
  void EmitZerofill(llvm::MCSection* section, llvm::MCSymbol* symbol,
                    uint64_t size, unsigned byte_alignment) override {}

  std::vector<llvm::MCInst>* const Result_;
};

}  // namespace

std::vector<llvm::MCInst> ParseAsmCodeFromFile(
    const GlobalContext& Context, const std::string& FileName,
    const llvm::InlineAsm::AsmDialect Dialect) {
  auto MemBuf = llvm::MemoryBuffer::getFileOrSTDIN(FileName);
  if (!MemBuf) {
    std::cerr << "could not open asm file\n";
    return {};
  }
  llvm::SourceMgr SM;
  SM.AddNewSourceBuffer(std::move(MemBuf.get()), llvm::SMLoc());

  std::vector<llvm::MCInst> Result;

  MCInstStreamer Streamer(Context.LLVMContext.get(), &Result);
  const std::unique_ptr<llvm::MCAsmParser> AsmParser(llvm::createMCAsmParser(
      SM, *Context.LLVMContext, Streamer, *Context.AsmInfo));
  AsmParser->setAssemblerDialect(Dialect);

  const std::unique_ptr<llvm::MCTargetAsmParser> TargetAsmParser(
      Context.Target->createMCAsmParser(*Context.SubtargetInfo, *AsmParser,
                                        *Context.InstrInfo,
                                        llvm::MCTargetOptions()));

  if (!TargetAsmParser) {
    std::cerr << "failed to create target assembly parser.\n";
    return {};
  }
  AsmParser->setTargetParser(*TargetAsmParser);

  if (AsmParser->Run(false)) {
    std::cerr << "could not parse asm file\n";
    return {};
  }
  return Result;
}

void TextTable::Render(llvm::raw_ostream& OS) const {
  OS << "\n";
  // Compute the width of each column.
  std::vector<int> Widths((NumCols_));
  for (int Row = 0; Row < NumRows(); ++Row) {
    for (int Col = 0; Col < NumCols_; ++Col) {
      const auto ValueWidth = Values_[Row * NumCols_ + Col].size();
      if (ValueWidth > Widths[Col]) {
        Widths[Col] = ValueWidth;
      }
    }
  }

  RenderSeparator(Widths, OS);
  int Row = 0;
  if (HasHeader_) {
    RenderRow(Row, Widths, OS);
    RenderSeparator(Widths, OS);
    ++Row;
  }

  for (; Row < NumRows(); ++Row) {
    RenderRow(Row, Widths, OS);
  }
  RenderSeparator(Widths, OS);
}

void TextTable::RenderSeparator(const std::vector<int>& Widths,
                                llvm::raw_ostream& OS) const {
  for (const int Width : Widths) {
    for (int I = 0; I < Width + 3; ++I) {
      OS << '-';
    }
  }
  OS << "-\n";
}

void TextTable::RenderRow(const int Row, const std::vector<int>& Widths,
                          llvm::raw_ostream& OS) const {
  for (int Col = 0; Col < NumCols_; ++Col) {
    const auto& Value = Values_[Row * NumCols_ + Col];
    OS << "| ";
    OS << llvm::right_justify(Value, Widths[Col]);
    OS << ' ';
  }
  OS << "| ";
  OS << TrailingValues_[Row];
  OS << "\n";
}

}  // namespace simulator
}  // namespace exegesis
