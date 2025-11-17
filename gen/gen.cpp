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

namespace opt {

std::string target;
std::string mcu;

} // namespace opt

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

std::string
getEffectiveTargetTriple()
{
    using namespace opt;

    if (!target.empty() && mcu.empty()) {
	return target;
    }
    if (target.empty() && !mcu.empty()) {
	if (mcu.starts_with("atmega") || mcu.starts_with("attiny")) {
	    llvm::errs() << "note: inferring target 'avr' from MCU '" << mcu
	                 << "'\n";
	    return "avr";
	} else if (mcu.starts_with("cortex-")) {
	    llvm::errs() << "note: inferring target 'arm-none-eabi' from MCU '"
	                 << mcu << "'\n";
	    return "arm-none-eabi";
	} else {
	    llvm::errs() << "warning: unknown MCU '" << mcu
	                 << "', ignoring --mmcu\n";
	    return llvm::sys::getDefaultTargetTriple();
	}
    }
    if (!target.empty() && !mcu.empty()) {
	if (target == "avr" &&
	    (mcu.starts_with("atmega") || mcu.starts_with("attiny"))) {
	    return target;
	}
	if (target == "arm-none-eabi" && mcu.starts_with("cortex-")) {
	    return target;
	}

	llvm::errs() << "warning: target '" << target
	             << "' does not match MCU '" << mcu << "', ignoring both\n";
	return llvm::sys::getDefaultTargetTriple();
    }
    return llvm::sys::getDefaultTargetTriple();
}

std::string
getCpu()
{
    using namespace opt;

    if (opt::mcu.empty())
	return "generic";

    if (mcu.starts_with("atmega") || mcu.starts_with("attiny"))
	return mcu;
    if (mcu.starts_with("cortex-"))
	return mcu.substr(7); // "cortex-m3" → "m3"

    llvm::errs() << "warning: unknown MCU '" << mcu << "', using generic CPU\n";
    return "generic";
}

std::string
getFeatures()
{
    return "";
}

llvm::Reloc::Model
getRelocModel(const std::string &targetTriple)
{
    return (targetTriple == "avr" || targetTriple == "arm-none-eabi")
               ? llvm::Reloc::Static
               : llvm::Reloc::PIC_;
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
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto tripleStr = getEffectiveTargetTriple();
    llvm::Triple TT(tripleStr);
    llvmModule->setTargetTriple(TT);

    std::string error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(TT, error);
    if (!target) {
	llvm::errs() << error;
	std::exit(1);
    }

    llvm::TargetOptions topts{};
    auto relocModel = getRelocModel(tripleStr);
    auto codeModel = std::optional<llvm::CodeModel::Model>();
    llvm::CodeGenOptLevel cgOpt = mapOpt(optLevel);

    auto cpu = getCpu();
    targetMachine = target->createTargetMachine(TT, cpu, getFeatures(), topts,
                                                relocModel, codeModel, cgOpt);

    llvmModule->setDataLayout(targetMachine->createDataLayout());
}

llvm::OptimizationLevel
getOptimizationLevel()
{
    return optimizationLevel;
}

} // namespace gen
