#ifndef EXEGESIS_LLVM_INLINE_ASM_H_
#define EXEGESIS_LLVM_INLINE_ASM_H_

#include <stdint.h>
#include <memory>
#include <vector>
#include "strings/string.h"

#include "glog/logging.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/SourceMgr.h"

namespace exegesis {

// Represents a function that takes no arguments and does not return any value.
struct VoidFunction {
  using Pointer = void (*)();

  static VoidFunction Undefined() { return VoidFunction(nullptr, 0); }

  VoidFunction() = delete;
  VoidFunction(Pointer ptr, int size) : ptr(ptr), size(size) {}
  void CallOrDie() const {
    DCHECK(ptr);
    ptr();
  }
  bool IsValid() const { return ptr != nullptr; }

  const Pointer ptr;
  const int size;
};

// A simple JIT compiler class that enables to assemble code at run time,
// encapsulate it into a loop, compile it and get a pointer to the corresponding
// function.

class JitCompiler {
 public:
  enum ErrorHandlingMode {
    // If the compiler encounters an error in the inline assembly, the program
    // prints an error message to STDERR and exits.
    EXIT_ON_ERROR,
    // If the compiler encounters an error in the inline assembly, the function
    // prints an error message to the log and returns nullptr instead of the
    // function pointer.
    RETURN_NULLPTR_ON_ERROR,
  };

  // Creates a Jit compiler parsing the dialect of x86 assembly given by
  // 'dialect'.
  // mcpu is the CPU used for compiling the inline assembly. The value must be
  // one of the CPU microarchitecture names accepted by LLVM. To get the full
  // list, run "blaze run @llvm_git//:llc -- --mcpu=help". Picking
  // the most generic processor ("generic" for x86) means that the generated
  // code will be able to run all hosts, but that the compiler wil refuse to
  // compile newer instructions (since all processors might not support them).
  JitCompiler(llvm::InlineAsm::AsmDialect dialect, const string& mcpu,
              ErrorHandlingMode error_mode);

  // Builds, compiles and returns a pointer to a void() function that executes
  // a loop of 'num_iterations' around 'loop_code'. Registers touched by
  // 'constraints' are saved, and the compiler assumes that the function does
  // have side effects.
  // Returns nullptr if some error has occurred. (This should be checked by
  // the client.)
  VoidFunction CompileInlineAssemblyToFunction(
      int num_iterations, const std::string& loop_code,
      const std::string& loop_constraints);

  // A version of CompileInlineAssemblyToFunction that accepts (1) a block of
  // initialization assembly code that is executed once at the beginning of the
  // function, and (2) a block of assembly code that is executed in the loop.
  // Both blocks of code require constraint specification. Note that the LLVM
  // code generator still has some freedom in how it allocates the registers,
  // and their values might not be preserved between the initialization and the
  // loop unless they are properly annotated in both sets of constraints.
  VoidFunction CompileInlineAssemblyToFunction(
      int num_iterations, const std::string& init_code,
      const std::string& init_constraints, const std::string& loop_code,
      const std::string& loop_constraints, const std::string& cleanup_code,
      const std::string& cleanup_constraints);

  // Builds, compiles and returns a pointer to the assembled code of
  // 'code'. This is not a function, and it should not be cast and
  // called: Registers are not saved, and the compiler assumes that
  // the code does not have side effects.
  // Returns nullptr if some error has occurred. (This should be checked by
  // the client.)
  uint8_t* CompileInlineAssemblyFragment(const std::string& code);

  // Returns an object usable by the LLVM IR that corresponds to the inline
  // assembly code in 'code' with constraints in 'constraints'.
  // When 'has_side_effects' is true, the inline assembler issues the code to
  // save some registers.
  llvm::InlineAsm* AssembleInlineNativeCode(bool has_side_effects,
                                            const std::string& code,
                                            const std::string& constraints);

  // Builds a LLVM IR function object that loops over the block of inline
  // assembly in 'inline_asm'. The number of iterations in the loop is given by
  // 'num_iterations'. Optionally, the function also accepts blocks of inline
  // assembly that is called once at the beginning (resp. at the end) of the
  // function to initialize (resp. clean up) the memory and the registers. Any
  // of these two values can be null to disable this feature.
  // Returns nullptr if some error has occurred. (This should be checked by
  // the client.)
  llvm::Function* WarpInlineAsmInLoopingFunction(
      int num_iterations, llvm::Value* init_inline_asm,
      llvm::Value* loop_inline_asm, llvm::Value* cleanup_inline_asm);

  // Builds and compiles a void() function that executes the LLVM IR function
  // passed in function. Compiles the function using 'execution_engine_',
  // and returns a pointer to the compiled function.
  // Returns nullptr if some error has occurred. (This should be checked by
  // the client.)
  VoidFunction CreatePointerToInlineAssemblyFunction(llvm::Function* function);

  // For debugging purposes. Dumps all the modules in the current object.
  void DumpAllModules();

 private:
  class StoreSizeMemoryManager;

  // Initializes the JitCompiler. Must be done before doing anything. Called
  // automatically by each of the member functions if necessary.
  void Init();

  static void HandleDiagnostic(const llvm::DiagnosticInfo& diagnostic,
                               void* context);
  static void HandleInlineAsmDiagnostic(const llvm::SMDiagnostic& diagnostic,
                                        void* context, unsigned loc_cookie);

  const string mcpu_;
  ErrorHandlingMode error_mode_;

  // 'dialect' is either AD_ATT or AD_Intel, refering to the two assembly
  // language dialects for the Intel architectures.
  llvm::InlineAsm::AsmDialect dialect_;

  std::unique_ptr<llvm::LLVMContext> context_;

  // This is owned and deleted by context_.
  llvm::Module* module_;

  // An instance of MCJIT (or another execution engine that can handle
  // inline assembly, and that supports ExecutionEngine::getFunctionAddress).
  std::unique_ptr<llvm::ExecutionEngine> execution_engine_;
  // The memory manager for execution_engine_.
  const StoreSizeMemoryManager* memory_manager_;

  // This is a place-holder for the void function type.
  llvm::FunctionType* function_type_;

  // Class JitCompiler numbers the functions one after the other.
  int function_id_ = 0;

  // Holds whether the object was initialized.
  bool initialized_ = false;

  // The list of compiler error messages from inline assembly collected during
  // the build.
  std::vector<string> compile_errors_;
};

}  // namespace exegesis

#endif  // EXEGESIS_LLVM_INLINE_ASM_H_
