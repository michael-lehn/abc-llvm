#ifndef GEN_INIT_HPP
#define GEN_INIT_HPP

#include "llvm/IR/LLVMContext.h"

namespace gen {

static std::unique_ptr<llvm::LLVMContext> context;

void init();

} // namespace gen

#endif // GEN_INIT_HPP
