#include "exegesis/llvm/inline_asm.h"

#include <unordered_map>

#include "exegesis/llvm/llvm_utils.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"  // IWYU pragma: keep
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/SourceMgr.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/gtl/ptr_util.h"

namespace exegesis {

// A memory manager that stores the size of the blocks it allocates. This is
// used to retrieve get the size of generated code.
class JitCompiler::StoreSizeMemoryManager : public llvm::SectionMemoryManager {
 public:
  // Returns the size of the section starting at `address`.
  int GetSectionSize(const uint8_t* const address) const {
    return FindOrDieNoPrint(address_to_size_, address);
  }

 private:
  uint8_t* allocateCodeSection(uintptr_t size, unsigned alignment,
                               unsigned section_id,
                               llvm::StringRef section_name) override {
    uint8_t* const result = llvm::SectionMemoryManager::allocateCodeSection(
        size, alignment, section_id, section_name);
    // We should never allocate a block of memory twice.
    InsertOrDieNoPrint(&address_to_size_, result, size);
    return result;
  }

  std::unordered_map<const uint8_t*, int> address_to_size_;
};

JitCompiler::JitCompiler(llvm::InlineAsm::AsmDialect dialect,
                         const string& mcpu, ErrorHandlingMode error_mode)
    : mcpu_(mcpu), error_mode_(error_mode), dialect_(dialect) {}

void JitCompiler::Init() {
  initialized_ = true;
  EnsureLLVMWasInitialized();
  context_.reset(new llvm::LLVMContext());
  if (error_mode_ == RETURN_NULLPTR_ON_ERROR) {
    context_->setInlineAsmDiagnosticHandler(
        &JitCompiler::HandleInlineAsmDiagnostic, this);
    context_->setDiagnosticHandler(&JitCompiler::HandleDiagnostic, this,
                                   /*RespectFilters=*/true);
  }
  module_ = new llvm::Module("Temp Module for JIT", *context_);
  CHECK(module_ != nullptr);
  auto memory_manager = gtl::MakeUnique<StoreSizeMemoryManager>();
  memory_manager_ = memory_manager.get();
  execution_engine_.reset(
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_))
          .setMCPU(MakeStringRef(mcpu_))
          .setMCJITMemoryManager(std::move(memory_manager))
          .create());
  CHECK(execution_engine_ != nullptr);
  llvm::Type* const void_type = llvm::Type::getVoidTy(*context_);
  CHECK(void_type != nullptr);
  function_type_ = llvm::FunctionType::get(void_type, false);
  CHECK(function_type_ != nullptr);
}

VoidFunction JitCompiler::CompileInlineAssemblyToFunction(
    int num_iterations, const std::string& loop_code,
    const std::string& loop_constraints) {
  if (!initialized_) Init();
  llvm::Value* loop_inline_asm =
      AssembleInlineNativeCode(true, loop_code, loop_constraints);
  CHECK(loop_inline_asm != nullptr);
  llvm::Function* loop = WarpInlineAsmInLoopingFunction(
      num_iterations, nullptr, loop_inline_asm, nullptr);
  CHECK(loop != nullptr);
  return CreatePointerToInlineAssemblyFunction(loop);
}

VoidFunction JitCompiler::CompileInlineAssemblyToFunction(
    int num_iterations, const std::string& init_code,
    const std::string& init_constraints, const std::string& loop_code,
    const std::string& loop_constraints, const std::string& cleanup_code,
    const std::string& cleanup_constraints) {
  if (!initialized_) Init();
  llvm::Value* const init_inline_asm =
      AssembleInlineNativeCode(true, init_code, init_constraints);
  CHECK(init_inline_asm != nullptr);
  llvm::Value* const loop_inline_asm =
      AssembleInlineNativeCode(true, loop_code, loop_constraints);
  CHECK(loop_inline_asm != nullptr);
  llvm::Value* const cleanup_inline_asm =
      AssembleInlineNativeCode(true, cleanup_code, init_constraints);
  CHECK(cleanup_inline_asm != nullptr);

  llvm::Function* const loop = WarpInlineAsmInLoopingFunction(
      num_iterations, init_inline_asm, loop_inline_asm, cleanup_inline_asm);
  CHECK(loop != nullptr);
  return CreatePointerToInlineAssemblyFunction(loop);
}

uint8_t* JitCompiler::CompileInlineAssemblyFragment(const std::string& code) {
  if (!initialized_) Init();
  llvm::Value* inline_asm = AssembleInlineNativeCode(false, code, "");
  CHECK(inline_asm != nullptr);
  llvm::Function* loop =
      WarpInlineAsmInLoopingFunction(1, nullptr, inline_asm, nullptr);
  CHECK(loop != nullptr);
  VoidFunction function = CreatePointerToInlineAssemblyFunction(loop);
  return reinterpret_cast<uint8_t*>(function.ptr);
}

llvm::InlineAsm* JitCompiler::AssembleInlineNativeCode(
    bool has_side_effects, const std::string& code,
    const std::string& constraints) {
  if (!initialized_) Init();
  return llvm::InlineAsm::get(function_type_, code, constraints,
                              has_side_effects, false, dialect_);
}

llvm::Function* JitCompiler::WarpInlineAsmInLoopingFunction(
    int num_iterations, llvm::Value* init_inline_asm,
    llvm::Value* loop_inline_asm, llvm::Value* cleanup_inline_asm) {
  if (!initialized_) Init();
  CHECK_GE(num_iterations, 1);
  constexpr char kModuleNameBase[] = "inline_assembly_module_";
  constexpr char kFunctionNameBase[] = "inline_assembly_";
  const std::string module_name = StrCat(kModuleNameBase, function_id_);
  const std::string function_name = StrCat(kFunctionNameBase, function_id_);
  ++function_id_;
  llvm::Module* module = new llvm::Module(module_name, *context_);
  llvm::Function* function = llvm::Function::Create(
      function_type_, llvm::Function::ExternalLinkage, function_name, module);
  if (function == nullptr) {
    LOG(DFATAL) << "Could not warp asm code in loop.\n";
    return function;
  }
  llvm::BasicBlock* function_basic_block =
      llvm::BasicBlock::Create(*context_, "entry", function);

  llvm::IRBuilder<> builder(*context_);

  builder.SetInsertPoint(function_basic_block);
  if (init_inline_asm != nullptr) {
    builder.CreateCall(init_inline_asm);
  }
  if (num_iterations == 1) {
    builder.CreateCall(loop_inline_asm, {});
  } else {
    llvm::BasicBlock* const loop_body =
        llvm::BasicBlock::Create(*context_, "loop", function);
    // Create the entry point; enter the body of the loop from there.
    builder.CreateBr(loop_body);

    // Create the body of the loop.
    builder.SetInsertPoint(loop_body);

    // We only handle ints.
    llvm::Type* const int_type = llvm::Type::getInt32Ty(*context_);

    llvm::PHINode* const counter_phi =
        builder.CreatePHI(int_type, 2, "counter");

    builder.CreateCall(loop_inline_asm, {});

    // Create a constant equal to one.
    llvm::Value* const const_one = llvm::ConstantInt::getSigned(int_type, 1);
    llvm::Value* const decremented_counter =
        builder.CreateSub(counter_phi, const_one, "new_counter");

    llvm::Value* const initial_counter_value =
        llvm::ConstantInt::getSigned(int_type, num_iterations);
    counter_phi->addIncoming(initial_counter_value, function_basic_block);
    counter_phi->addIncoming(decremented_counter, loop_body);

    // Create a constant equal to zero.
    llvm::Value* const const_zero = llvm::ConstantInt::getSigned(int_type, 0);
    llvm::Value* const is_greater =
        builder.CreateICmpSGT(decremented_counter, const_zero);

    llvm::BasicBlock* const loop_end =
        llvm::BasicBlock::Create(*context_, "loop_end", function);
    builder.CreateCondBr(is_greater, loop_body, loop_end);

    // Create return function
    builder.SetInsertPoint(loop_end);
  }
  if (cleanup_inline_asm != nullptr) {
    builder.CreateCall(cleanup_inline_asm);
  }
  builder.CreateRetVoid();
  llvm::verifyFunction(*function);

  return function;
}

VoidFunction JitCompiler::CreatePointerToInlineAssemblyFunction(
    llvm::Function* function) {
  compile_errors_.clear();
  if (!initialized_) Init();
  llvm::Module* const module = function->getParent();
  if (module == nullptr) {
    LOG(DFATAL) << "Module not found";
    return VoidFunction::Undefined();
  }
  execution_engine_->addModule(std::unique_ptr<llvm::Module>(module));

  // Find the function by name (it was added to the new module when it was
  // created, and adding the module to the execution engine is enough to get it
  // here), and compile it at the same time.
  //
  // NOTE(ondrasej): getFunctionAddress only works with MCJIT (and not with JIT
  // or the interpreter), but we don't care, because JIT and the interpreter
  // cannot execute inline assembly anyway.
  const std::string function_name = function->getName();
  uint64_t function_ptr = execution_engine_->getFunctionAddress(function_name);
  if (function_ptr == 0) {
    LOG(DFATAL)
        << "getFunctionAddress returned nullptr. Are you sure you use MCJIT?";
  }
  if (!compile_errors_.empty()) {
    for (const string& error : compile_errors_) {
      LOG(ERROR) << error;
    }
    return VoidFunction::Undefined();
  }
  return VoidFunction(reinterpret_cast<void (*)()>(function_ptr),
                      memory_manager_->GetSectionSize(
                          reinterpret_cast<const uint8_t*>(function_ptr)));
}

void JitCompiler::HandleDiagnostic(const llvm::DiagnosticInfo& diagnostic,
                                   void* context) {
  JitCompiler* const self = static_cast<JitCompiler*>(context);
  std::string error_message;
  llvm::raw_string_ostream error_message_ostream(error_message);
  llvm::DiagnosticPrinterRawOStream error_message_printer(
      error_message_ostream);
  diagnostic.print(error_message_printer);
  self->compile_errors_.push_back(error_message);
}

void JitCompiler::HandleInlineAsmDiagnostic(
    const llvm::SMDiagnostic& diagnostic, void* context, unsigned loc_cookie) {
  JitCompiler* const self = static_cast<JitCompiler*>(context);
  std::string error_message;
  llvm::raw_string_ostream error_message_ostream(error_message);
  diagnostic.print(nullptr, error_message_ostream);
  self->compile_errors_.push_back(error_message_ostream.str());
}

}  // namespace exegesis
