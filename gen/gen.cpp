#include "gen.hpp"

namespace gen {

std::unique_ptr<llvm::LLVMContext> llvmContext;
std::unique_ptr<llvm::Module> llvmModule;
std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
llvm::BasicBlock *llvmBB;

const char *moduleName;

void
init(const char *name)
{
    moduleName = name ? name : "llvm";
    llvmContext = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>(moduleName, *llvmContext);
    llvmBuilder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);
}

} // namespace gen
