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

// A IACA-like simulator (main).

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm_sim/analysis/inverse_throughput.h"
#include "llvm_sim/analysis/port_pressure.h"
#include "llvm_sim/x86/faucon_lib.h"
#include "llvm_sim/x86/haswell.h"

static llvm::cl::opt<std::string> LogFile(
    "log", llvm::cl::desc("Write simulation log to file"),
    llvm::cl::value_desc("log_file"), llvm::cl::init(""), llvm::cl::NotHidden);

static llvm::cl::opt<std::string> TraceFile(
    "trace", llvm::cl::desc("Write simulation trace to file"),
    llvm::cl::value_desc("trace_file"), llvm::cl::init(""),
    llvm::cl::NotHidden);

static llvm::cl::opt<std::string> InputFile(llvm::cl::Positional,
                                            llvm::cl::desc("<input file>"),
                                            llvm::cl::Required);

static llvm::cl::opt<int> MaxIters(
    "max_iters", llvm::cl::desc("Maximum number of iterations"),
    llvm::cl::value_desc("num"), llvm::cl::init(20), llvm::cl::NotHidden);

static llvm::cl::opt<int> MaxCycles("max_cycles",
                                    llvm::cl::desc("Maximum number of cycles"),
                                    llvm::cl::value_desc("num"),
                                    llvm::cl::init(100000),
                                    llvm::cl::NotHidden);

static llvm::cl::opt<bool> IsLoopBody(
    "loop_body", llvm::cl::desc("Whether the code is in a loop body"),
    llvm::cl::init(true), llvm::cl::NotHidden);

enum class InputFileTypeE { Bin, AsmIntel, AsmATT };
static llvm::cl::opt<InputFileTypeE> InputFileType(
    "input_type", llvm::cl::desc("input file type"),
    llvm::cl::values(
        clEnumValN(InputFileTypeE::Bin, "bin", "IACA-marked binary"),
        clEnumValN(InputFileTypeE::AsmIntel, "intel_asm", "Intel assembly"),
        clEnumValN(InputFileTypeE::AsmATT, "att_asm", "AT&T assembly")));

namespace exegesis {
namespace simulator {
namespace {

void PrintPortPressures(const GlobalContext& Context,
                        const BlockContext& BlockContext,
                        const SimulationLog& Log,
                        llvm::MCInstPrinter& AsmPrinter) {
  // Compute port pressures.
  const auto PortPressures = ComputePortPressure(BlockContext, Log);

  // Display global port pressure.
  {
    std::cout << "\nPort Pressure (cycles per iteration):\n";
    TextTable Table(2, PortPressures.Pressures.size() + 1, true);
    Table.SetValue(0, 0, "Port");
    Table.SetValue(1, 0, "Cycles");
    for (int I = 0; I < PortPressures.Pressures.size(); ++I) {
      Table.SetValue(
          0, I + 1,
          Log.BufferDescriptions[PortPressures.Pressures[I].BufferIndex]
              .DisplayName);
      const auto Pressure = PortPressures.Pressures[I].CyclesPerIteration;
      if (Pressure == 0.0f) {
        continue;  // Do not print the 0 for unused ports.
      }
      char buf[32];
      snprintf(buf, sizeof(buf), "%0.2f", Pressure);
      Table.SetValue(1, I + 1, buf);
    }
    Table.Render(llvm::outs());
  }
  std::cout << "\n";

  std::cout << "* - some instruction uops do not use a resource\n";

  // Display port pressure per instruction.
  {
    constexpr const int UopsCol = 0;
    TextTable Table(BlockContext.GetNumBasicBlockInstructions() + 1,
                    PortPressures.Pressures.size() + 1, true);
    // Write header.
    Table.SetValue(0, UopsCol, "#Uops");
    for (int I = 0; I < PortPressures.Pressures.size(); ++I) {
      Table.SetValue(
          0, I + 1,
          Log.BufferDescriptions[PortPressures.Pressures[I].BufferIndex]
              .DisplayName);
    }
    // Write instruction port pressures.
    for (unsigned InstrIdx = 0;
         InstrIdx < BlockContext.GetNumBasicBlockInstructions(); ++InstrIdx) {
      const int CurTableRow = InstrIdx + 1;
      const auto Uops = Context
                            .GetInstructionDecomposition(
                                BlockContext.GetInstruction(InstrIdx))
                            .Uops;
      std::string UopString;
      if (std::any_of(Uops.begin(), Uops.end(),
                      [](const InstrUopDecomposition::Uop& Uop) {
                        return Uop.ProcResIdx == 0;
                      })) {
        UopString.push_back('*');
      }
      UopString.append(std::to_string(Uops.size()));
      Table.SetValue(CurTableRow, UopsCol, UopString);
      for (int I = 0; I < PortPressures.Pressures.size(); ++I) {
        const auto Pressure =
            PortPressures.Pressures[I].CyclesPerIterationByMCInst[InstrIdx];
        if (Pressure == 0.0f) {
          continue;  // Do not print the 0 for unused ports.
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%0.2f", Pressure);
        Table.SetValue(CurTableRow, I + 1, buf);
      }
      std::string InstrString;
      llvm::raw_string_ostream OS(InstrString);
      AsmPrinter.printInst(&BlockContext.GetInstruction(InstrIdx), 0, "",
                           *Context.SubtargetInfo, OS);
      OS.flush();
      Table.SetTrailingValue(CurTableRow, InstrString);
    }
    Table.Render(llvm::outs());
  }
}

int Simulate() {
  const auto Context = GlobalContext::Create("x86_64", "haswell");
  if (!Context) {
    return EXIT_FAILURE;
  }
  const auto Simulator = CreateHaswellSimulator(*Context);

  std::cout << "analyzing '" << InputFile << "'\n";
  std::vector<llvm::MCInst> Instructions;
  switch (InputFileType) {
    case InputFileTypeE::Bin:
      Instructions = ParseIACAMarkedCodeFromFile(*Context, InputFile);
      break;
    case InputFileTypeE::AsmIntel:
      Instructions =
          ParseAsmCodeFromFile(*Context, InputFile, llvm::InlineAsm::AD_Intel);
      break;
    case InputFileTypeE::AsmATT:
      Instructions =
          ParseAsmCodeFromFile(*Context, InputFile, llvm::InlineAsm::AD_ATT);
      break;
  }
  std::cout << "analyzing " << Instructions.size() << " instructions\n";
  const BlockContext BlockContext(Instructions, IsLoopBody);

  const auto Log = Simulator->Run(BlockContext, MaxIters, MaxCycles);

  std::cout << "ran " << Log->Iterations.size() << " iterations in "
            << Log->NumCycles << " cycles\n";

  // Optionally write log to file.
  if (!LogFile.empty()) {
    std::ofstream OFS(LogFile);
    OFS << Log->DebugString();
  }

  constexpr const unsigned kIntelSyntax = 1;
  const std::unique_ptr<llvm::MCInstPrinter> AsmPrinter(
      Context->Target->createMCInstPrinter(
          Context->Triple, kIntelSyntax, *Context->AsmInfo, *Context->InstrInfo,
          *Context->RegisterInfo));
  AsmPrinter->setPrintImmHex(true);

  // Optionally write trace to file.
  if (!TraceFile.empty()) {
    std::error_code ErrorCode;
    llvm::raw_fd_ostream OFS(TraceFile, ErrorCode,
                             llvm::sys::fs::CD_CreateAlways,
                             llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write,
                             llvm::sys::fs::OpenFlags::OF_Text);
    if (ErrorCode) {
      std::cerr << "Cannot write trace file: " << ErrorCode << "\n";
    } else {
      PrintTrace(*Context, BlockContext, *Log, *AsmPrinter, OFS);
    }
  }

  if (Log->Iterations.empty()) {
    return 0;
  }

  const auto InvThroughput = ComputeInverseThroughput(BlockContext, *Log);
  std::cout << "Block Inverse Throughput (last " << InvThroughput.NumIterations
            << " iterations): [" << InvThroughput.Min << "-"
            << InvThroughput.Max << "] cycles per iteration, "
            << InvThroughput.TotalNumCycles << " cycles total\n";

  PrintPortPressures(*Context, BlockContext, *Log, *AsmPrinter);

  return 0;
}

}  // namespace
}  // namespace simulator
}  // namespace exegesis

int main(int argc, char** argv) {
  if (!llvm::cl::ParseCommandLineOptions(argc, argv, "", &llvm::errs())) {
    return EXIT_FAILURE;
  }

  LLVMInitializeX86Target();
  LLVMInitializeX86TargetInfo();
  LLVMInitializeX86TargetMC();
  LLVMInitializeX86Disassembler();
  LLVMInitializeX86AsmParser();

  return exegesis::simulator::Simulate();
}
