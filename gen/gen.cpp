#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"

#include "gen.hpp"

namespace gen {

std::unique_ptr<llvm::LLVMContext> llvmContext;
std::unique_ptr<llvm::Module> llvmModule;
std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
llvm::BasicBlock *llvmBB;
llvm::TargetMachine *targetMachine;

const char *moduleName;

void
init(const char *name)
{
    moduleName = name ? name : "llvm";
    llvmContext = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>(moduleName, *llvmContext);
    llvmBuilder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeNativeTarget();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
	llvm::errs() << error;
	std::exit(1);
    }

    int codeGenOptLevel = 0;

    targetMachine = target->createTargetMachine(
	    targetTriple,
	    "generic",
	    "",
	    llvm::TargetOptions{},
	    llvm::Reloc::PIC_,
	    std::nullopt,
	    llvm::CodeGenOpt::Level{codeGenOptLevel});
    llvmModule->setDataLayout(targetMachine->createDataLayout());
}

} // namespace gen
