#ifndef GEN_INIT_HPP
#define GEN_INIT_HPP

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

namespace gen {

extern std::unique_ptr<llvm::LLVMContext> llvmContext;
extern std::unique_ptr<llvm::Module> llvmModule;
extern std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
extern llvm::BasicBlock *llvmBB;

void init();

} // namespace gen

#endif // GEN_INIT_HPP
