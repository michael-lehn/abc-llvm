#ifndef GEN_GEN_HPP
#define GEN_GEN_HPP

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Target/TargetMachine.h"

namespace gen {

using Label = llvm::BasicBlock *;
using JumpOrigin = llvm::BasicBlock *;
using Value = llvm::Value *;
using Constant = llvm::Constant *;
using ConstantInt = llvm::ConstantInt *;

extern std::unique_ptr<llvm::LLVMContext> llvmContext;
extern std::unique_ptr<llvm::Module> llvmModule;
extern std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
extern llvm::BasicBlock *llvmBB;
extern llvm::TargetMachine *targetMachine;

extern const char *moduleName;

void init(const char *name = nullptr,
	  llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O0);
llvm::OptimizationLevel  getOptimizationLevel();

} // namespace gen

#endif // GEN_GEN_HPP
