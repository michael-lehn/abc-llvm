#include <iostream>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include "gen.hpp"
#include "gentype.hpp"
#include "variable.hpp"

namespace gen {

std::unique_ptr<llvm::LLVMContext> llvmContext;
std::unique_ptr<llvm::Module> llvmModule;
std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
llvm::BasicBlock *llvmBB;
llvm::TargetMachine *targetMachine;

const char *moduleName;
static llvm::OptimizationLevel optimizationLevel;

static inline llvm::CodeGenOptLevel
mapOpt(llvm::OptimizationLevel L)
{
    using OL = llvm::OptimizationLevel;
    if (L == OL::O0)
	return llvm::CodeGenOptLevel::None;
    if (L == OL::O1)
	return llvm::CodeGenOptLevel::Less;
    if (L == OL::O2)
	return llvm::CodeGenOptLevel::Default;
    if (L == OL::O3)
	return llvm::CodeGenOptLevel::Aggressive;
    // Fallback
    return llvm::CodeGenOptLevel::Default;
}

void
init(const char *name, llvm::OptimizationLevel optLevel)
{
    optimizationLevel = optLevel;
    forgetAllVariables();
    initTypeMap();
    moduleName = name ? name : "llvm";

    if (!llvmContext) {
	llvmContext = std::make_unique<llvm::LLVMContext>();
    }
    llvmModule = std::make_unique<llvm::Module>(moduleName, *llvmContext);
    llvmBuilder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);
    llvmBB = nullptr;

    llvm::InitializeAllTargetInfos();
    llvm::InitializeNativeTarget();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    auto tripleStr = llvm::sys::getDefaultTargetTriple();
#if LLVM_VERSION_MAJOR >= 21
    llvm::Triple TT(tripleStr);
    llvmModule->setTargetTriple(TT);
#else
    llvmModule->setTargetTriple(tripleStr);
    llvm::Triple TT(tripleStr);
#endif

    std::string error;
#if LLVM_VERSION_MAJOR >= 21
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(TT, error);
#else
    const llvm::Target *target =
        llvm::TargetRegistry::lookupTarget(tripleStr, error);
#endif
    if (!target) {
	llvm::errs() << error;
	std::exit(1);
    }

    llvm::TargetOptions topts{};
    auto relocModel = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    auto codeModel = std::optional<llvm::CodeModel::Model>();
    llvm::CodeGenOptLevel cgOpt = mapOpt(optLevel);

#if LLVM_VERSION_MAJOR >= 21
    targetMachine = target->createTargetMachine(TT,
                                                /*CPU*/ "generic",
                                                /*Features*/ "", topts,
                                                relocModel, codeModel, cgOpt);
#else
    targetMachine = target->createTargetMachine(tripleStr,
                                                /*CPU*/ "generic",
                                                /*Features*/ "", topts,
                                                relocModel, codeModel, cgOpt);
#endif

    llvmModule->setDataLayout(targetMachine->createDataLayout());
}

llvm::OptimizationLevel
getOptimizationLevel()
{
    return optimizationLevel;
}

} // namespace gen
