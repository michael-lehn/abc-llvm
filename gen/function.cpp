#include <cstring>
#include <iostream>

#include "llvm/IR/Verifier.h"

#include "constant.hpp"
#include "convert.hpp"
#include "function.hpp"
#include "gen.hpp"
#include "instruction.hpp"
#include "label.hpp"
#include "variable.hpp"

namespace gen {

FunctionBuildingInfo functionBuildingInfo;

llvm::Function *
functionDeclaration(const char *ident, const abc::Type *fnType,
		    bool externalLinkage)
{
    assert(llvmContext);
    if (auto fn = llvmModule->getFunction(ident)) {
	// already declared
	/*
	 * Whether a function is external or not is specified by its
	 * first declaration. Like in C:
	 * - An extern declaration can be followed by a static declaration
	 * - A static static declaration *can not* be followed by an extern
	 *   declaration.
	 */
	assert(!externalLinkage
		    || fn->getLinkage() == llvm::Function::ExternalLinkage);
	return fn;
    }

    auto linkage = externalLinkage || !strcmp(ident, "main")
	? llvm::Function::ExternalLinkage
	: llvm::Function::InternalLinkage;

    auto llvmFnType = llvm::dyn_cast<llvm::FunctionType>(convert(fnType));

    auto fn = llvm::Function::Create(llvmFnType,
				     linkage,
				     ident,
				     llvmModule.get());
    return fn;
}

void
functionDefinitionBegin(const char *ident, const abc::Type *fnType,
			const std::vector<const char *> &param,
			bool externalLinkage)
{
    assert(param.size() == fnType->paramType().size());

    auto fn = functionDeclaration(ident, fnType, externalLinkage);
    fn->setDoesNotThrow();
    llvmBB = llvm::BasicBlock::Create(*llvmContext, "entry", fn);
    llvmBuilder->SetInsertPoint(llvmBB);

    auto retType = fnType->retType();

    functionBuildingInfo.fn = fn; 
    functionBuildingInfo.leave = getLabel(".leave");
    functionBuildingInfo.retType = retType;
    functionBuildingInfo.retVal = nullptr;
    functionBuildingInfo.bbClosed = false;

    for (std::size_t i = 0; i < param.size(); ++i) {
	auto addr = localVariableDefinition(param[i], fnType->paramType()[i]);
	store(fn->getArg(i), addr);
    }

    if (!retType->isVoid()) {
	functionBuildingInfo.retVal = localVariableDefinition(".retVal",
							      retType);
	if (!strcmp(ident, "main")) {
	    store(getConstantInt("0", retType), functionBuildingInfo.retVal);
	}
    }
}

void
functionDefinitionEnd()
{
    assert(llvmBuilder);
    assert(functionBuildingInfo.fn);
    assert(functionBuildingInfo.leave);

    if (!functionBuildingInfo.bbClosed) {
	jumpInstruction(functionBuildingInfo.leave);
    }
    defineLabel(functionBuildingInfo.leave);

    if (functionBuildingInfo.retType->isVoid()) {
	llvmBuilder->CreateRetVoid();
    } else {
	auto retVal = fetch(functionBuildingInfo.retVal,
			    functionBuildingInfo.retType);
	llvmBuilder->CreateRet(retVal);
    }
    llvm::verifyFunction(*functionBuildingInfo.fn);

    functionBuildingInfo.fn = nullptr; 
    functionBuildingInfo.leave = nullptr; 
    functionBuildingInfo.retType = nullptr;
    functionBuildingInfo.retVal = nullptr;
    functionBuildingInfo.bbClosed = true;
}

Value
functionCall(Value fnAddr, const abc::Type *fnType,
	     const std::vector<Value> &arg)
{
    assert(fnType);
    auto llvmFnType = llvm::dyn_cast<llvm::FunctionType>(convert(fnType));
    assert(llvmFnType);
    return llvmBuilder->CreateCall(llvmFnType, fnAddr, arg);
}

} // namespace gen
