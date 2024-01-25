#include <iostream>
#include <system_error>
#include <unordered_map>
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


static std::unordered_map<std::string, llvm::GlobalValue *> global;


int
main(void)
{
    static std::unique_ptr<llvm::LLVMContext> TheContext;
    static std::unique_ptr<llvm::Module> TheModule;
    static std::unique_ptr<llvm::IRBuilder<>> Builder;

    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("llvm test", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    auto structType = llvm::StructType::create(
			"Foo",
			llvm::Type::getInt8Ty(*TheContext),
			llvm::Type::getInt16Ty(*TheContext),
			llvm::Type::getInt64Ty(*TheContext)
			);


    global["fooVar"] = new llvm::GlobalVariable(*TheModule, structType,
			/*isConstant=*/false,
			/*Linkage=*/llvm::GlobalValue::ExternalLinkage,
			/*Initializer=*/nullptr,
			/*Name=*/"fooVar");

    /*
     * Create function 'void main(void)'
     */
    llvm::Type *retType = llvm::Type::getVoidTy(*TheContext);
    std::vector<llvm::Type *> argType;
    llvm::FunctionType *fnType = llvm::FunctionType::get(
				    retType, argType, false);
    llvm::Function *fn = llvm::Function::Create(fnType,
						llvm::Function::ExternalLinkage,
						"main", TheModule.get());
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*TheContext, "entry", fn);
    Builder->SetInsertPoint(bb);
    /*
     * Function body
     */
    std::vector<llvm::Value *> idxList{2};
    idxList[0] = llvm::ConstantInt::get(*TheContext, llvm::APInt(8, "0", 10));
    idxList[1] = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, "2", 10));

    auto addr = Builder->CreateGEP(structType, global["fooVar"], idxList);

    auto val =  llvm::ConstantInt::get(*TheContext, llvm::APInt(64, "42", 10));
    Builder->CreateStore(val, addr);
    auto s = Builder->CreateLoad(structType, addr);
    Builder->CreateStore(s, addr);


    /*
     * Create return;
     */
    Builder->CreateRetVoid();
    TheModule->print(llvm::errs(), nullptr);
}
