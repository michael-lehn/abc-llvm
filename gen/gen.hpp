#ifndef GEN_GEN_HPP
#define GEN_GEN_HPP

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
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

// for optimization
extern std::unique_ptr<llvm::FunctionPassManager> llvmFPM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> llvmFAM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> llvmFAM;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> llvmCGAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> llvmMAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> llvmPIC;
extern std::unique_ptr<llvm::StandardInstrumentations> llvmSI;

extern const char *moduleName;

void init(const char *name = nullptr, int optimizationLevel = 0);
int  getOptimizationLevel();

} // namespace gen

#endif // GEN_GEN_HPP
