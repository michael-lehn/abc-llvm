#ifndef GEN_GEN_HPP
#define GEN_GEN_HPP

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

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

void init();

} // namespace gen

#endif // GEN_GEN_HPP
