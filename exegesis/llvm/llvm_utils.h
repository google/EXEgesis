// Contains helper function for interacting with LLVM subsystems.

#ifndef EXEGESIS_LLVM_LLVM_UTILS_H_
#define EXEGESIS_LLVM_LLVM_UTILS_H_

#include <vector>
#include "strings/string.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/ScheduleDAG.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Value.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "strings/string_view.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::StatusOr;

// Ensures that LLVM subsystems were initialized for instruction scheduling.
// This function can be called safely multiple times, all calls except for the
// first one are effectively no-ops.
// TODO(ondrasej): Pass some command-line flags to LLVM?
void EnsureLLVMWasInitialized();

// Looks up the LLVM target based on the command-line flags passed to the
// program or the default LLVM target for the current architecture; the target
// is the target for the triple returned by GetNormalizedLLVMTripleName. The
// returned pointer is not owned by the caller and it must not be deleted.
StatusOr<const llvm::Target*> GetLLVMTarget();

// Returns normalized LLVM Triple name based on the current platform and the
// command-line flags.
std::string GetNormalizedLLVMTripleName();

// Creates a human-readable string representation of a LLVM IR object. This
// function can be used for example to get the IR code of llvm::Function in a
// string form.
std::string DumpIRToString(const llvm::Value* ir);

// Creates a human-readable string representation of 'instruction' that can be
// used e.g. for logging.
std::string DumpMachineInstrToString(const llvm::MachineInstr* instruction);

// Creates a human-readable string representation of an operand that can be used
// e.g. for logging.
std::string DumpMachineOperandToString(const llvm::MachineOperand* operand);

// Creates a human-readable string representation of the scheduling unit that
// can be used e.g. for logging.
std::string DumpMachineInstrSUnitToString(const llvm::SUnit* sunit);

// Creates a human-readable string representation of an MC instruction object
// that can be used e.g. for logging.
std::string DumpMCInstToString(const llvm::MCInst* instruction);

// Creates a human-readable string representation of the MC instruction object.
// Unlike the version with a single argument above, this function translates
// instruction and register codes to their symbolic names.
string DumpMCInstToString(const llvm::MCInst& instruction,
                          const llvm::MCInstrInfo* mc_instruction_info,
                          const llvm::MCRegisterInfo* register_info);

// Creates a human-readable string representation of the scheduling dependency.
string DumpSDepToString(const llvm::SDep& sdep);

// Creates a human-readable string representation of the MC operand object.
string DumpMCOperandToString(const llvm::MCOperand& operand,
                             const llvm::MCRegisterInfo* register_info);

// Creates a human-readable string representation of 'mem_operand' that can be
// used e.g. for logging.
std::string DumpMachineMemOperandToString(
    const llvm::MachineMemOperand* mem_operand);

// Returns the list of X86 LLVM instruction mnemonics.
std::vector<string> GetLLVMMnemonicListOrDie();

llvm::StringRef MakeStringRef(const string& source);
llvm::StringRef MakeStringRef(StringPiece source);

// Parses the asm dialect from a human-readable string representation. Accepted
// values are "att" and "intel".
StatusOr<llvm::InlineAsm::AsmDialect> ParseAsmDialectName(
    const string& asm_dialect_name);

// Wrapper around LLVM InitTargetOptionsFromCodeGenFlags to avoid linker issues.
llvm::TargetOptions LLVMInitTargetOptionsFromCodeGenFlags();

// Wrapper around LLVM InitMCTargetOptionsFromFlags to avoid linker issues.
llvm::MCTargetOptions LLVMInitMCTargetOptionsFromFlags();

}  // namespace exegesis

#endif  // EXEGESIS_LLVM_LLVM_UTILS_H_
