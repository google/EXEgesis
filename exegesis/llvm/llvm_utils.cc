#include "exegesis/llvm/llvm_utils.h"

#include <stdint.h>
#include <cstdlib>
#include <memory>
#include <vector>
#include "strings/string.h"

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCTargetOptionsCommandFlags.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "strings/case.h"
#include "strings/str_cat.h"
#include "util/task/canonical_errors.h"

DEFINE_string(exegesis_llvm_arch, "",
              "The architecture, for which the code is compiled.");
DEFINE_string(exegesis_llvm_triple, "",
              "The LLVM triple, for which the code is compiled.");

namespace exegesis {

using ::google::ProgramUsage;

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::StatusOr;

void EnsureLLVMWasInitialized() {
  static const bool dummy = []() {
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86TargetMC();
    llvm::PassRegistry* const pass_registry =
        llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(*pass_registry);
    llvm::initializeCodeGen(*pass_registry);
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86AsmParser();
    LLVMInitializeX86Disassembler();

    // Create (fake) command-line options for LLVM. We use this to inject our
    // own dumping instruction scheduler without modifying too much LLVM code.
    // TODO(ondrasej): Remove this hack (will need either re-writing a big part
    // of the codegen path we are using, or patchign LLVM sources to support
    // "true" dependency injection).
    const std::vector<const char*> argv = {
        ProgramUsage() /*, "--pre-RA-sched=dumper"*/
    };
    llvm::cl::ParseCommandLineOptions(argv.size(), argv.data(), ProgramUsage());
    VLOG(1) << "LLVM was initialized";
    return true;
  }();
  CHECK(dummy);  // Silence unused variable warning.
}

StatusOr<const llvm::Target*> GetLLVMTarget() {
  const std::string triple_name = FLAGS_exegesis_llvm_triple.empty()
                                      ? llvm::sys::getDefaultTargetTriple()
                                      : std::string(FLAGS_exegesis_llvm_triple);
  llvm::Triple triple(GetNormalizedLLVMTripleName());

  std::string error_message;
  const llvm::Target* const target = llvm::TargetRegistry::lookupTarget(
      FLAGS_exegesis_llvm_arch, triple, error_message);
  if (target == nullptr) {
    return InvalidArgumentError(error_message);
  }

  return target;
}

std::string GetNormalizedLLVMTripleName() {
  const std::string triple_name = FLAGS_exegesis_llvm_triple.empty()
                                      ? llvm::sys::getDefaultTargetTriple()
                                      : std::string(FLAGS_exegesis_llvm_triple);
  return llvm::Triple::normalize(triple_name);
}

namespace {
template <typename Object>
inline std::string DumpObjectToString(const Object* object) {
  CHECK(object != nullptr);
  std::string buffer;
  llvm::raw_string_ostream stream(buffer);
  stream << *object;
  stream.flush();
  return buffer;
}
}  // namespace

std::string DumpIRToString(const llvm::Value* ir) {
  return DumpObjectToString(ir);
}

std::string DumpMachineInstrToString(const llvm::MachineInstr* instruction) {
  return DumpObjectToString(instruction);
}

std::string DumpMachineMemOperandToString(
    const llvm::MachineMemOperand* mem_operand) {
  return DumpObjectToString(mem_operand);
}

std::string DumpMachineOperandToString(const llvm::MachineOperand* operand) {
  return DumpObjectToString(operand);
}

std::string DumpMachineInstrSUnitToString(const llvm::SUnit* sunit) {
  CHECK(sunit != nullptr);
  CHECK(sunit->isInstr());
  return DumpObjectToString(sunit->getInstr());
}

std::string DumpMCInstToString(const llvm::MCInst* instruction) {
  CHECK(instruction != nullptr);
  return DumpObjectToString(instruction);
}

#define ADD_SDEP_PROPERTY_TO_BUFFER(sdep, property, buffer) \
  if (sdep.property()) {                                    \
    *buffer += ", " #property;                              \
  }

string DumpSDepToString(const llvm::SDep& sdep) {
  string buffer = "SDep: ";
  StrAppend(&buffer, "\n  Kind: ", sdep.getKind());
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isNormalMemory, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isBarrier, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isMustAlias, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isWeak, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isArtificial, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isCluster, &buffer);
  ADD_SDEP_PROPERTY_TO_BUFFER(sdep, isAssignedRegDep, &buffer);
  if (sdep.getSUnit() && sdep.getSUnit()->isInstr()) {
    StrAppend(&buffer, "\n  Other SUnit: ",
              DumpMachineInstrSUnitToString(sdep.getSUnit()));
  }
  return buffer;
}

#undef ADD_SDEP_PROPERTY_TO_BUFFER

string DumpMCOperandToString(const llvm::MCOperand& operand,
                             const llvm::MCRegisterInfo* register_info) {
  CHECK(register_info != nullptr);
  // TODO(ondrasej): We also need to detect memory operands properly. However,
  // this might be tricky because this information is not represented explicitly
  // in the LLVM MC layer.
  string debug_string;
  if (operand.isValid()) {
    if (operand.isImm()) {
      debug_string = StrCat("Imm(", operand.getImm(), ")");
    } else if (operand.isFPImm()) {
      debug_string = StrCat("FPImm(", operand.getFPImm(), ")");
    } else if (operand.isReg()) {
      debug_string = StrCat("R:", register_info->getName(operand.getReg()), "(",
                            operand.getReg(), ")");
    } else if (operand.isExpr()) {
      debug_string = "expr";
    } else if (operand.isInst()) {
      debug_string = "inst";
    } else {
      debug_string = "unknown";
    }
  } else {
    debug_string = "invalid";
  }
  return debug_string;
}

string DumpMCInstToString(const llvm::MCInst& instruction,
                          const llvm::MCInstrInfo* mc_instruction_info,
                          const llvm::MCRegisterInfo* register_info) {
  CHECK(mc_instruction_info != nullptr);
  CHECK(register_info != nullptr);
  const unsigned opcode = instruction.getOpcode();
  const llvm::MCInstrDesc& instruction_descriptor =
      mc_instruction_info->get(opcode);

  string debug_string =
      mc_instruction_info->getName(instruction.getOpcode()).str();
  for (int i = 0; i < instruction.getNumOperands(); ++i) {
    const llvm::MCOperand& operand = instruction.getOperand(i);
    StrAppend(&debug_string, " ",
              DumpMCOperandToString(operand, register_info));
  }
  StrAppend(&debug_string, ", ", instruction_descriptor.getNumDefs(),
            " def(s)");
  StrAppend(&debug_string, ", ", instruction_descriptor.getNumOperands(),
            " operand(s)");
  if (instruction_descriptor.mayStore()) {
    StrAppend(&debug_string, ", may store");
  }
  if (instruction_descriptor.mayLoad()) {
    StrAppend(&debug_string, ", may load");
  }

  string implicit_defs_str;
  const uint16_t* const implicit_defs =
      instruction_descriptor.getImplicitDefs();
  for (int i = 0; i < instruction_descriptor.getNumImplicitDefs(); ++i) {
    const uint16_t implicit_def_register = implicit_defs[i];
    StrAppend(&implicit_defs_str, " ",
              register_info->getName(implicit_def_register));
  }
  StrAppend(&debug_string, ", implicit def:", implicit_defs_str);

  string implicit_uses_str;
  const uint16_t* const implicit_uses =
      instruction_descriptor.getImplicitUses();
  for (int i = 0; i < instruction_descriptor.getNumImplicitUses(); ++i) {
    const uint16_t implicit_use_register = implicit_uses[i];
    StrAppend(&implicit_uses_str, " ",
              register_info->getName(implicit_use_register));
  }
  StrAppend(&debug_string, ", implicit use: ", implicit_uses_str);

  return debug_string;
}

std::vector<string> GetLLVMMnemonicListOrDie() {
  EnsureLLVMWasInitialized();
  std::string error_message;
  const llvm::Target* target = llvm::TargetRegistry::lookupTarget(
      llvm::sys::getDefaultTargetTriple(), error_message);
  CHECK(target != nullptr) << error_message;

  std::unique_ptr<llvm::MCInstrInfo> instr_info(target->createMCInstrInfo());
  CHECK(instr_info != nullptr);

  const int num_opcodes = instr_info->getNumOpcodes();
  std::vector<string> mnemonics;
  mnemonics.reserve(num_opcodes);
  for (int i = 0; i < num_opcodes; ++i) {
    mnemonics.emplace_back(instr_info->getName(i));
  }
  return mnemonics;
}

llvm::StringRef MakeStringRef(const string& source) {
  return llvm::StringRef(source.data(), source.size());
}

llvm::StringRef MakeStringRef(StringPiece source) {
  return llvm::StringRef(source.data(), source.size());
}

StatusOr<llvm::InlineAsm::AsmDialect> ParseAsmDialectName(
    const string& asm_dialect_name) {
  const string canonical_name = strings::ToUpper(asm_dialect_name);
  if (canonical_name == "INTEL") {
    return ::llvm::InlineAsm::AD_Intel;
  } else if (canonical_name == "ATT" || canonical_name == "AT&T") {
    return ::llvm::InlineAsm::AD_ATT;
  } else {
    return InvalidArgumentError(
        StrCat("Unknown assembly dialect '", asm_dialect_name, "'"));
  }
}

llvm::TargetOptions LLVMInitTargetOptionsFromCodeGenFlags() {
  return InitTargetOptionsFromCodeGenFlags();
}

llvm::MCTargetOptions LLVMInitMCTargetOptionsFromFlags() {
  return InitMCTargetOptionsFromFlags();
}

}  // namespace exegesis
