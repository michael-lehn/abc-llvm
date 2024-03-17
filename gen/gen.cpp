#include <iostream>

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

#include "gen.hpp"

namespace gen {

std::unique_ptr<llvm::LLVMContext> llvmContext;
std::unique_ptr<llvm::Module> llvmModule;
std::unique_ptr<llvm::IRBuilder<>> llvmBuilder;
llvm::BasicBlock *llvmBB;
llvm::TargetMachine *targetMachine;

// for optimization
std::unique_ptr<llvm::FunctionPassManager> llvmFPM;
std::unique_ptr<llvm::LoopAnalysisManager> llvmLAM;
std::unique_ptr<llvm::FunctionAnalysisManager> llvmFAM;
std::unique_ptr<llvm::CGSCCAnalysisManager> llvmCGAM;
std::unique_ptr<llvm::ModuleAnalysisManager> llvmMAM;
std::unique_ptr<llvm::PassInstrumentationCallbacks> llvmPIC;
std::unique_ptr<llvm::StandardInstrumentations> llvmSI;


const char *moduleName;
static int optimizationLevel;

void
init(const char *name, int optimizationLevel)
{
    moduleName = name ? name : "llvm";
    llvmContext = std::make_unique<llvm::LLVMContext>();
    llvmModule = std::make_unique<llvm::Module>(moduleName, *llvmContext);
    llvmBuilder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);

    if (optimizationLevel > 0) {
	assert(!llvmFPM);

	llvmFPM = std::make_unique<llvm::FunctionPassManager>();
	assert(llvmFPM);
	llvmLAM = std::make_unique<llvm::LoopAnalysisManager>();
	assert(llvmLAM);
	llvmFAM = std::make_unique<llvm::FunctionAnalysisManager>();
	assert(llvmFAM);
	llvmCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
	assert(llvmCGAM);
	llvmMAM = std::make_unique<llvm::ModuleAnalysisManager>();
	assert(llvmMAM);
	llvmPIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
	assert(llvmPIC);
	llvmSI = std::make_unique<llvm::StandardInstrumentations>(
			*llvmContext, /*DebugLogging*/ true);
	assert(llvmSI);

	llvmSI->registerCallbacks(*llvmPIC, llvmMAM.get());
	assert(llvmSI);
	llvmFPM->addPass(llvm::InstCombinePass());
	assert(llvmFPM);
	llvmFPM->addPass(llvm::ReassociatePass());
	assert(llvmFPM);
	llvmFPM->addPass(llvm::GVNPass());
	assert(llvmFPM);
	llvmFPM->addPass(llvm::SimplifyCFGPass());
	assert(llvmFPM);
    }

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

    targetMachine = target->createTargetMachine(
	    targetTriple,
	    "generic",
	    "",
	    llvm::TargetOptions{},
	    llvm::Reloc::PIC_,
	    std::nullopt,
	    llvm::CodeGenOpt::Level{optimizationLevel});
    llvmModule->setDataLayout(targetMachine->createDataLayout());

    if (optimizationLevel > 0) {
	llvm::PassBuilder pb{targetMachine};
	pb.registerModuleAnalyses(*llvmMAM);
	pb.registerLoopAnalyses(*llvmLAM);
	pb.registerFunctionAnalyses(*llvmFAM);
	pb.registerCGSCCAnalyses(*llvmCGAM);
	pb.registerFunctionAnalyses(*llvmFAM);
	pb.crossRegisterProxies(*llvmLAM, *llvmFAM, *llvmCGAM, *llvmMAM);
    }
}

int
getOptimizationLevel()
{
    return optimizationLevel;
}

} // namespace gen
