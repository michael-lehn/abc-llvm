#include <system_error>
#include <memory>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

int
main(void)
{
    static std::unique_ptr<llvm::LLVMContext> TheContext;
    static std::unique_ptr<llvm::Module> TheModule;
    static std::unique_ptr<llvm::IRBuilder<>> Builder;

    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("llvm test", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    llvm::Type *retType = llvm::Type::getInt8Ty(*TheContext);

    std::vector<llvm::Type *> argType{3};
    argType[0] = llvm::Type::getInt8Ty(*TheContext);
    argType[1] = llvm::Type::getInt64Ty(*TheContext);
    argType[2] = llvm::Type::getDoubleTy(*TheContext);

    llvm::FunctionType *fnType = llvm::FunctionType::get(retType, argType,
							 false);
    llvm::Function *fnDecl = llvm::Function::Create(fnType,
						llvm::Function::ExternalLinkage,
						"bar", TheModule.get());

    llvm::Function *fn = llvm::Function::Create(fnType,
						llvm::Function::ExternalLinkage,
						"foo", TheModule.get());
    fn->setDoesNotThrow();

    //fn->getArg(0)->setName("a");
    //fn->getArg(1)->setName("b");
    //fn->getArg(2)->setName("c");

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*TheContext, "entry", fn);
    Builder->SetInsertPoint(bb);

    llvm::AllocaInst *Alloca = Builder->CreateAlloca(
			    argType[0], //fn->getArg(0)->getParamByValType(),
			    nullptr,
			    "param0");
    Builder->CreateStore(fn->getArg(0), Alloca);


    auto arg0 = fn->getArg(0);
    auto arg1 = fn->getArg(1);
    auto c0 = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, "123", 10));
    auto tmp1 = Builder->CreateAdd(c0, arg1);
    auto tmp2 = Builder->CreateAdd(c0, c0);
    auto tmp3 = Builder->CreateAdd(arg0, tmp2);
    auto tmp4 = Builder->CreateAdd(tmp3, tmp3);
    auto tmp5 = Builder->CreateAdd(tmp3, tmp3);
    Builder->CreateRet(Builder->CreateAdd(tmp4, tmp5));

    llvm::GlobalVariable* gvar_ptr_abc = new llvm::GlobalVariable(
	 *TheModule,
        llvm::Type::getInt64Ty(*TheContext),
        /*isConstant=*/false,
        /*Linkage=*/llvm::GlobalValue::ExternalLinkage,
        /*Initializer=*/llvm::dyn_cast<llvm::ConstantInt>(tmp2),
        /*Name=*/"abc");

    TheModule->print(llvm::errs(), nullptr);


    // Generate object file ...

    // Initialize the target registry etc.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
	llvm::errs() << Error;
	return 1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    //opt.EnableCFIFixup = 0;
    //opt.DebugStrictDwarf = 0;

    auto TheTargetMachine = Target->createTargetMachine(
	TargetTriple, CPU, Features, opt, llvm::Reloc::PIC_,
	std::nullopt
	, llvm::CodeGenOpt::Aggressive
	);

    TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    //auto Filename = "output.o";
    auto Filename = "output.s";
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
	llvm::errs() << "Could not open file: " << EC.message();
	return 1;
    }

    llvm::legacy::PassManager pass;
    //auto FileType = llvm::CodeGenFileType::CGFT_ObjectFile;
    auto FileType = llvm::CodeGenFileType::CGFT_AssemblyFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
	llvm::errs() << "TheTargetMachine can't emit a file of this type";
	return 1;
    }

    pass.run(*TheModule);
    dest.flush();

    llvm::outs() << "Wrote " << Filename << "\n";
}
