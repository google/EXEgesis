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

// A IACA-like simulator (library).

#ifndef EXEGESIS_LLVM_SIM_TEST_FAUCON_LIB_H_
#define EXEGESIS_LLVM_SIM_TEST_FAUCON_LIB_H_

#include <cstdint>
#include <string>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm_sim/framework/context.h"
#include "llvm_sim/framework/log.h"

namespace exegesis {
namespace simulator {

// Parses the code in between IACA markers in the given binary file. Returns an
// empty vector on error.
std::vector<llvm::MCInst> ParseIACAMarkedCodeFromFile(
    const GlobalContext& Context, const std::string& FileName);

// Parses the asm code in the given text file. Returns an empty vector on error.
std::vector<llvm::MCInst> ParseAsmCodeFromFile(
    const GlobalContext& Context, const std::string& FileName,
    llvm::InlineAsm::AsmDialect Dialect);

// Parses the asm code from a string. Returns an empty vector on error.
std::vector<llvm::MCInst> ParseAsmCodeFromString(
    const GlobalContext& Context, const std::string& Assembly,
    llvm::InlineAsm::AsmDialect Dialect);

// Returns the code in between IACA markers. Returns an empty vector on error.
std::vector<uint8_t> GetIACAMarkedCode(const std::string& FileName);

// Parse `CodeBytes` into MCInsts.
std::vector<llvm::MCInst> ParseMCInsts(const GlobalContext& Context,
                                       llvm::ArrayRef<uint8_t> CodeBytes);

// Prints a IACA-style execution trace.
void PrintTrace(const GlobalContext& Context, const BlockContext& BlockContext,
                const SimulationLog& Log, llvm::MCInstPrinter& AsmPrinter,
                llvm::raw_ostream& OS);

// A class to represent a table and render it out to an llvm::raw_ostream.
class TextTable {
 public:
  TextTable(int NumRows, int NumCols, bool HasHeader)
      : NumCols_(NumCols),
        HasHeader_(HasHeader),
        Values_(NumCols * NumRows),
        TrailingValues_(NumRows) {}

  // Sets the value of cell (Row, Col).
  void SetValue(int Row, int Col, const std::string& Value) {
    Values_[Row * NumCols_ + Col] = Value;
  }

  // Sets a trailing value which is rendered after the row.
  void SetTrailingValue(int Row, const std::string& Value) {
    TrailingValues_[Row] = Value;
  }

  int NumRows() const { return Values_.size() / NumCols_; }
  int NumCols() const { return NumCols_; }

  void Render(llvm::raw_ostream& OS) const;

 private:
  void RenderRow(int Row, const std::vector<int>& Widths,
                 llvm::raw_ostream& OS) const;
  void RenderSeparator(const std::vector<int>& Widths,
                       llvm::raw_ostream& OS) const;

  // Number of columns.
  const int NumCols_;
  const bool HasHeader_;
  // Row-major values.
  std::vector<std::string> Values_;
  // Trailing values, one per row.
  std::vector<std::string> TrailingValues_;
};

}  // namespace simulator
}  // namespace exegesis

#endif  // EXEGESIS_LLVM_SIM_TEST_FAUCON_LIB_H_
