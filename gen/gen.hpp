#ifndef GEN_GEN_HPP
#define GEN_GEN_HPP

#include <string>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/Constants.h"
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
using ConstantFloat = llvm::ConstantFP *;

extern std::unique_ptr<llvm::LLVMContext> llvmContext;
extern std::unique_ptr<llvm::Module> llvmModule;
extern std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
extern llvm::BasicBlock *llvmBB;
extern llvm::TargetMachine *targetMachine;

namespace opt {

extern std::string target;
extern std::string mcu;

// 0: optimize for speed, 1: optimize for size (-Os), 2: minimize size (-Oz)
extern unsigned sizeLevel;

} // namespace opt

extern const char *moduleName;

void init(const char *name = nullptr,
          llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O0);

// Functions declared without 'extern' have internal linkage, so their definition
// has to be in this file. Removes such a declaration if nothing uses it. Returns
// false if something does, with the reason on stderr.
bool checkDeclaredFunctions();

// false if the generated IR is invalid, with the reason on stderr
bool verifyModule();

std::size_t getLongBits(); // number of bits of C's long on the target
llvm::OptimizationLevel getOptimizationLevel();

} // namespace gen

#endif // GEN_GEN_HPP
