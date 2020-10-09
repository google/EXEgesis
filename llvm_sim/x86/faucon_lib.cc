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
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm_sim/x86/constants.h"

namespace exegesis {
namespace simulator {

namespace {

// IACA magic markers.
// NOTE(ondrasej): This format is required for GCC 7.3.0 which does not accept
// numbers over 127 as char literals. On the other hand, this can't be an
// unsigned char array, because SectionRef::getContents returns the data as
// StringRef and std::search would do char/unsigned char comparisons which are
// ill-defined when char is signed.
constexpr char kBeginMagicMarker[] = {'\x0f', '\x0b', '\xbb', '\x6f', '\x00',
                                      '\x00', '\x00', '\x64', '\x67', '\x90'};
constexpr char kEndMagicMarker[] = {'\xbb', '\xde', '\x00', '\x00', '\x00',
                                    '\x64', '\x67', '\x90', '\x0f', '\x0b'};

}  // namespace

std::vector<uint8_t> GetIACAMarkedCode(const std::string& FileName) {
  // Open binary and search for marker.
  auto BinaryOrErr = llvm::object::createBinary(FileName);
  if (!BinaryOrErr) {
    std::cerr << "could not open binary: " << FileName << "\n";
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
    llvm::Expected<llvm::StringRef> Code = Section.getContents();
    // Find the begin/end markers.
    auto BeginIt = std::search(Code->begin(), Code->end(), kBeginMagicMarker,
                               kBeginMagicMarker + sizeof(kBeginMagicMarker));
    if (BeginIt == Code->end()) {
      continue;
    }
    BeginIt += sizeof(kBeginMagicMarker);
    const auto EndIt = std::search(BeginIt, Code->end(), kEndMagicMarker,
                                   kEndMagicMarker + sizeof(kEndMagicMarker));
    if (EndIt == Code->end()) {
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
  while (Disasm->getInstruction(Inst, InstSize, CodeBytes, 0, llvm::nulls()) ==
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
  void emitInstruction(
      const llvm::MCInst& instruction,
      const llvm::MCSubtargetInfo& mc_subtarget_info) override {
    Result_->push_back(instruction);
  }

 private:
  // We only care about instructions, we don't implement this part of the API.
  void emitCommonSymbol(llvm::MCSymbol* symbol, uint64_t size,
                        unsigned byte_alignment) override {}
  bool emitSymbolAttribute(llvm::MCSymbol* symbol,
                           llvm::MCSymbolAttr attribute) override {
    return false;
  }
  void emitValueToAlignment(unsigned byte_alignment, int64_t value,
                            unsigned value_size,
                            unsigned max_bytes_to_emit) override {}
  void emitZerofill(llvm::MCSection* section, llvm::MCSymbol* symbol,
                    uint64_t size, unsigned byte_alignment,
                    llvm::SMLoc Loc) override {}

  std::vector<llvm::MCInst>* const Result_;
};

std::vector<llvm::MCInst> ParseAsmCodeFromMemoryBuffer(
    const GlobalContext& Context, std::unique_ptr<llvm::MemoryBuffer> MemBuf,
    const llvm::InlineAsm::AsmDialect Dialect) {
  llvm::SourceMgr SM;
  SM.AddNewSourceBuffer(std::move(MemBuf), llvm::SMLoc());

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

}  // namespace

std::vector<llvm::MCInst> ParseAsmCodeFromFile(
    const GlobalContext& Context, const std::string& FileName,
    const llvm::InlineAsm::AsmDialect Dialect) {
  auto MemBuf = llvm::MemoryBuffer::getFileOrSTDIN(FileName);
  if (!MemBuf) {
    std::cerr << "could not open asm file\n";
    return {};
  }
  return ParseAsmCodeFromMemoryBuffer(Context, std::move(MemBuf.get()),
                                      Dialect);
}

std::vector<llvm::MCInst> ParseAsmCodeFromString(
    const GlobalContext& Context, const std::string& Assembly,
    const llvm::InlineAsm::AsmDialect Dialect) {
  auto MemBuf = llvm::MemoryBuffer::getMemBuffer(Assembly);
  return ParseAsmCodeFromMemoryBuffer(Context, std::move(MemBuf), Dialect);
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

namespace {

// Represents a trace matrix (4th trace column), indexed by (Iteration, BBIndex,
// UopIndex).
struct TraceMatrix {
  explicit TraceMatrix(const GlobalContext& Context,
                       const BlockContext& BlockContext,
                       const SimulationLog& Log)
      : EmptyRow_(Log.NumCycles, ' ') {
    for (const auto& Line : Log.Lines) {
      switch (Log.BufferDescriptions[Line.BufferIndex].Id) {
        case IntelBufferIds::kAllocated:
          TryAssignState(Log, Line, 'A');
          break;
        case IntelBufferIds::kIssuePort:
          TryAssignState(Log, Line, 'd');
          break;
        case IntelBufferIds::kWriteback:
          TryAssignState(Log, Line, 'w');
          break;
        case IntelBufferIds::kRetired:
          TryAssignState(Log, Line, 'R');
          break;
        default:
          break;
      }
    }
  }

  const std::vector<char>& GetRow(unsigned Iteration, unsigned BBIndex,
                                  unsigned UopIndex) const {
    UopId::Type Uop = {{Iteration, BBIndex}, UopIndex};
    const auto It = UopToTrace_.find(Uop);
    return It == UopToTrace_.end() ? EmptyRow_ : It->second;
  }

 private:
  void TryAssignState(const SimulationLog& Log, const SimulationLog::Line& Line,
                      const char State) {
    if (Line.MsgTag != UopId::kTagName) {
      return;
    }
    UopId::Type Uop;
    llvm::StringRef Msg = Line.Msg;
    if (UopId::Consume(Msg, Uop)) {
      return;
    }
    if (Uop.InstrIndex.Iteration >= Log.GetNumCompleteIterations()) {
      // Ignore any incomplete iteration.
      return;
    }
    auto& MatrixRow = UopToTrace_[Uop];
    if (MatrixRow.empty()) {
      // This is the first time we hit this row, create it.
      MatrixRow = EmptyRow_;
    }
    MatrixRow[Line.Cycle] = State;
  }

  struct UopIdLess {
    bool operator()(const UopId::Type& Uop1, const UopId::Type& Uop2) const {
      return std::tie(Uop1.InstrIndex.Iteration, Uop1.InstrIndex.BBIndex,
                      Uop1.UopIndex) < std::tie(Uop2.InstrIndex.Iteration,
                                                Uop2.InstrIndex.BBIndex,
                                                Uop2.UopIndex);
    }
  };
  std::map<UopId::Type, std::vector<char>, UopIdLess> UopToTrace_;
  const std::vector<char> EmptyRow_;
};

struct TraceColumnWidths {
  explicit TraceColumnWidths(const BlockContext& BlockContext,
                             const SimulationLog& Log)
      : ItColWidth(std::max(
            2.0, std::ceil(std::log10(Log.GetNumCompleteIterations())))),
        InColWidth(
            std::max(2.0, std::ceil(std::log10(
                              BlockContext.GetNumBasicBlockInstructions())))) {}

  // Iteration column width.
  const unsigned ItColWidth;
  // Instruction column width.
  const unsigned InColWidth;
  // Disassembly column width.
  const unsigned DisassemblyColWidth = 50;
};

// Writes the first three columns of a line, justified according to
// `ColumnWidths`.
void WriteLineBegin(const TraceColumnWidths& ColumnWidths,
                    const llvm::StringRef ItColumn,
                    const llvm::StringRef InColumn,
                    const llvm::StringRef DisassmeblyColumn,
                    llvm::raw_ostream& OS) {
  OS << '|' << llvm::right_justify(ItColumn, ColumnWidths.ItColWidth) << '|'
     << llvm::right_justify(InColumn, ColumnWidths.InColWidth) << '|'
     << llvm::left_justify(DisassmeblyColumn, ColumnWidths.DisassemblyColWidth);
  OS << ':';
}

// The header looks like:
//   `|it  |in  |Disassembly    :012345678901234567890123`
void WriteTraceHeader(const SimulationLog& Log,
                      const TraceColumnWidths& ColumnWidths,
                      llvm::raw_ostream& OS) {
  WriteLineBegin(ColumnWidths, "it", "in", "Disassembly", OS);
  for (unsigned Cycle = 0; Cycle < Log.NumCycles; ++Cycle) {
    OS << (Cycle % 10);
  }
  OS << '\n';
}

// A raw_ostream that expands tabs.
class expandtabs_raw_ostream : public llvm::raw_ostream {
 public:
  explicit expandtabs_raw_ostream(llvm::raw_ostream& OS, unsigned TabWidth)
      : llvm::raw_ostream(/*Unbuffered=*/true),
        OS_(&OS),
        Tabs_(TabWidth, ' ') {}

  virtual ~expandtabs_raw_ostream() {}

 private:
  void write_impl(const char* Ptr, size_t Size) final {
    for (size_t I = 0; I < Size; ++I) {
      if (Ptr[I] == '\t') {
        OS_->write(Tabs_.data(), Tabs_.size());
      } else {
        OS_->write(Ptr + I, 1);
      }
    }
  }

  uint64_t current_pos() const final { return OS_->tell(); }

  llvm::raw_ostream* const OS_;
  const std::string Tabs_;
};

void WriteTraceLine(const GlobalContext& Context,
                    const BlockContext& BlockContext, const SimulationLog& Log,
                    llvm::MCInstPrinter& AsmPrinter, const TraceMatrix& Matrix,
                    const size_t Iter, const size_t BBIndex,
                    const TraceColumnWidths& ColumnWidths,
                    llvm::formatted_raw_ostream& OS) {
  // Get the instruction disassembly and expand tabs.
  std::string Disassembly;
  {
    llvm::raw_string_ostream SOS(Disassembly);
    expandtabs_raw_ostream ETOS(SOS, 4);
    AsmPrinter.printInst(&BlockContext.GetInstruction(BBIndex), 0, "",
                         *Context.SubtargetInfo, ETOS);
  }

  // First write the iteration, index and disassembly.
  WriteLineBegin(ColumnWidths, llvm::itostr(Iter), llvm::itostr(BBIndex),
                 Disassembly, OS);
  OS << "          ";
  for (unsigned Cycle = 10; Cycle < Log.NumCycles; ++Cycle) {
    OS << ((Cycle % 10) == 0 ? '|' : ' ');
  }
  OS << "\n";

  // Then each uop.
  const size_t NumUops =
      Context.GetInstructionDecomposition(BlockContext.GetInstruction(BBIndex))
          .Uops.size();
  for (size_t UopIndex = 0; UopIndex < NumUops; ++UopIndex) {
    WriteLineBegin(
        ColumnWidths, "", "",
        llvm::Twine("      uop ").concat(llvm::Twine(UopIndex)).str(), OS);
    const auto MatrixRow = Matrix.GetRow(Iter, BBIndex, UopIndex);
    char PrevState = ' ';
    for (unsigned Cycle = 0; Cycle < Log.NumCycles; ++Cycle) {
      char State = MatrixRow[Cycle];
      if (State == ' ') {
        // Fill in the blanks.
        if (PrevState != ' ' && PrevState != '|' && PrevState != 'R') {
          // Fill with '-' except when executing, where we fill with 'e'.
          State = (PrevState == 'e' || PrevState == 'd') ? 'e' : '-';
        } else if (Cycle > 0 && (Cycle % 10) == 0) {
          State = '|';
        }
      }
      OS << State;
      PrevState = State;
    }
    OS << "\n";
  }
}
}  // namespace

void PrintTrace(const GlobalContext& Context, const BlockContext& BlockContext,
                const SimulationLog& Log, llvm::MCInstPrinter& AsmPrinter,
                llvm::raw_ostream& OS) {
  const TraceMatrix Matrix(Context, BlockContext, Log);

  llvm::formatted_raw_ostream FOS(OS);

  const TraceColumnWidths ColumnWidths(BlockContext, Log);

  WriteTraceHeader(Log, ColumnWidths, FOS);

  for (size_t Iter = 0; Iter < Log.GetNumCompleteIterations(); ++Iter) {
    for (size_t BBIndex = 0;
         BBIndex < BlockContext.GetNumBasicBlockInstructions(); ++BBIndex) {
      WriteTraceLine(Context, BlockContext, Log, AsmPrinter, Matrix, Iter,
                     BBIndex, ColumnWidths, FOS);
    }
  }
}
}  // namespace simulator
}  // namespace exegesis
