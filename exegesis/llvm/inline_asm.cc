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

#include "exegesis/llvm/inline_asm.h"

#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "base/commandlineflags.h"
#include "exegesis/llvm/llvm_utils.h"
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
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"

namespace exegesis {

using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::InternalError;
using ::exegesis::util::InvalidArgumentError;

// A memory manager that stores the size of the blocks it allocates. This is
// used to retrieve get the size of generated code.
class JitCompiler::StoreSizeMemoryManager : public llvm::SectionMemoryManager {
 public:
  // Returns the size of the section starting at `address`.
  int GetSectionSize(const uint8_t* const address) const {
    return gtl::FindOrDieNoPrint(address_to_size_, address);
  }

 private:
  uint8_t* allocateCodeSection(uintptr_t size, unsigned alignment,
                               unsigned section_id,
                               llvm::StringRef section_name) override {
    uint8_t* const result = llvm::SectionMemoryManager::allocateCodeSection(
        size, alignment, section_id, section_name);
    // We should never allocate a block of memory twice.
    gtl::InsertOrDieNoPrint(&address_to_size_, result, size);
    return result;
  }

  absl::flat_hash_map<const uint8_t*, int> address_to_size_;
};

JitCompiler::JitCompiler(const std::string& mcpu) : mcpu_(mcpu) {}

void JitCompiler::Init() {
  initialized_ = true;
  EnsureLLVMWasInitialized();
  context_.reset(new llvm::LLVMContext());
  context_->setInlineAsmDiagnosticHandler(
      &JitCompiler::HandleInlineAsmDiagnostic, this);
  context_->setDiagnosticHandlerCallBack(&JitCompiler::HandleDiagnostic, this,
                                         /*RespectFilters=*/true);
  module_ = new llvm::Module("Temp Module for JIT", *context_);
  CHECK(module_ != nullptr);
  auto memory_manager = absl::make_unique<StoreSizeMemoryManager>();
  memory_manager_ = memory_manager.get();
  execution_engine_.reset(
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_))
          .setMCPU(mcpu_)
          .setMCJITMemoryManager(std::move(memory_manager))
          .create());

  // When trying to compile stuff like "mov eax, some_undefined_symbol", LLVM
  // LLVM will crash on lookup. Intercept the lookup and return a dummy address.
  execution_engine_->InstallLazyFunctionCreator(
      [this](const std::string& what) {
        intercepted_unknown_symbols_.push_back(what);
        return reinterpret_cast<void*>(1);
      });

  CHECK(execution_engine_ != nullptr);
  llvm::Type* const void_type = llvm::Type::getVoidTy(*context_);
  CHECK(void_type != nullptr);
  function_type_ = llvm::FunctionType::get(void_type, false);
  CHECK(function_type_ != nullptr);
}

StatusOr<VoidFunction> JitCompiler::CompileInlineAssemblyToFunction(
    int num_iterations, const std::string& loop_code,
    const std::string& loop_constraints, llvm::InlineAsm::AsmDialect dialect) {
  if (!initialized_) Init();
  llvm::Value* loop_inline_asm =
      AssembleInlineNativeCode(true, loop_code, loop_constraints, dialect);
  CHECK(loop_inline_asm != nullptr);
  const auto loop = WrapInlineAsmInLoopingFunction(num_iterations, nullptr,
                                                   loop_inline_asm, nullptr);
  if (!loop.ok()) {
    return loop.status();
  }
  return CreatePointerToInlineAssemblyFunction(loop.ValueOrDie());
}

StatusOr<VoidFunction> JitCompiler::CompileInlineAssemblyToFunction(
    int num_iterations, const std::string& init_code,
    const std::string& init_constraints, const std::string& loop_code,
    const std::string& loop_constraints, const std::string& cleanup_code,
    const std::string& cleanup_constraints,
    llvm::InlineAsm::AsmDialect dialect) {
  if (!initialized_) Init();
  llvm::Value* const init_inline_asm =
      AssembleInlineNativeCode(true, init_code, init_constraints, dialect);
  CHECK(init_inline_asm != nullptr);
  llvm::Value* const loop_inline_asm =
      AssembleInlineNativeCode(true, loop_code, loop_constraints, dialect);
  CHECK(loop_inline_asm != nullptr);
  llvm::Value* const cleanup_inline_asm =
      AssembleInlineNativeCode(true, cleanup_code, init_constraints, dialect);
  CHECK(cleanup_inline_asm != nullptr);

  const auto loop = WrapInlineAsmInLoopingFunction(
      num_iterations, init_inline_asm, loop_inline_asm, cleanup_inline_asm);
  if (!loop.ok()) {
    return loop.status();
  }
  return CreatePointerToInlineAssemblyFunction(loop.ValueOrDie());
}

StatusOr<uint8_t*> JitCompiler::CompileInlineAssemblyFragment(
    const std::string& code, llvm::InlineAsm::AsmDialect dialect) {
  if (!initialized_) Init();
  llvm::Value* inline_asm = AssembleInlineNativeCode(false, code, "", dialect);
  CHECK(inline_asm != nullptr);
  const auto loop =
      WrapInlineAsmInLoopingFunction(1, nullptr, inline_asm, nullptr);
  if (!loop.ok()) {
    return loop.status();
  }
  const auto function =
      CreatePointerToInlineAssemblyFunction(loop.ValueOrDie());
  if (!function.ok()) {
    return function.status();
  }
  return reinterpret_cast<uint8_t*>(function.ValueOrDie().ptr);
}

llvm::InlineAsm* JitCompiler::AssembleInlineNativeCode(
    bool has_side_effects, const std::string& code,
    const std::string& constraints, llvm::InlineAsm::AsmDialect dialect) {
  if (!initialized_) Init();
  return llvm::InlineAsm::get(function_type_, code, constraints,
                              has_side_effects, false, dialect);
}

StatusOr<llvm::Function*> JitCompiler::WrapInlineAsmInLoopingFunction(
    int num_iterations, llvm::Value* init_inline_asm,
    llvm::Value* loop_inline_asm, llvm::Value* cleanup_inline_asm) {
  if (!initialized_) Init();
  CHECK_GE(num_iterations, 1);
  constexpr char kModuleNameBase[] = "inline_assembly_module_";
  constexpr char kFunctionNameBase[] = "inline_assembly_";
  const std::string module_name = absl::StrCat(kModuleNameBase, function_id_);
  const std::string function_name =
      absl::StrCat(kFunctionNameBase, function_id_);
  ++function_id_;
  llvm::Module* module = new llvm::Module(module_name, *context_);
  llvm::Function* const function = llvm::Function::Create(
      function_type_, llvm::Function::ExternalLinkage, function_name, module);
  if (function == nullptr) {
    return InternalError("Could not create llvm Function object");
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
  std::string error_msg;
  ::llvm::raw_string_ostream os(error_msg);
  if (llvm::verifyFunction(*function, &os)) {
    return InternalError(
        absl::StrCat("llvm::verifyFunction failed: ", os.str()));
  }

  return function;
}

StatusOr<VoidFunction> JitCompiler::CreatePointerToInlineAssemblyFunction(
    llvm::Function* function) {
  compile_errors_.clear();
  intercepted_unknown_symbols_.clear();
  if (!initialized_) Init();
  llvm::Module* const module = function->getParent();
  if (module == nullptr) {
    return InternalError("Module not found");
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
    return FailedPreconditionError(
        "getFunctionAddress returned nullptr. Are you sure you use MCJIT?");
  }
  if (!compile_errors_.empty()) {
    return InvalidArgumentError(absl::StrJoin(compile_errors_, "; "));
  }
  if (!intercepted_unknown_symbols_.empty()) {
    return InvalidArgumentError(
        absl::StrCat("The following unknown symbols are referenced: '",
                     absl::StrJoin(intercepted_unknown_symbols_, "', '"), "'"));
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
