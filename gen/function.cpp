#include <cstring>
#include <iostream>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "abi.hpp"
#include "constant.hpp"
#include "function.hpp"
#include "gen.hpp"
#include "gentype.hpp"
#include "instruction.hpp"
#include "label.hpp"
#include "variable.hpp"

#include "type/integertype.hpp"

namespace gen {

FunctionBuildingInfo functionBuildingInfo;

bool
bbOpen()
{
    return !functionBuildingInfo.bbClosed;
}

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
	assert(!externalLinkage ||
	       fn->getLinkage() == llvm::Function::ExternalLinkage);
	return fn;
    }

    auto linkage = externalLinkage || !strcmp(ident, "main")
                       ? llvm::Function::ExternalLinkage
                       : llvm::Function::InternalLinkage;

    auto llvmFnType = llvm::dyn_cast<llvm::FunctionType>(convert(fnType));

    auto fn =
        llvm::Function::Create(llvmFnType, linkage, ident, llvmModule.get());

    // lower declaration
    auto abcParamType = fnType->paramType();
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.byVal) {
	    fn->addParamAttr(i, llvm::Attribute::getWithByValType(
	                            *llvmContext, convert(argInfo.byValType)));
	    fn->addParamAttr(i, llvm::Attribute::getWithAlignment(
	                            *llvmContext, argInfo.align));
	}
    }

    return fn;
}

void
functionDefinitionBegin(const char *ident, const abc::Type *fnType,
                        const std::vector<const char *> &param,
                        bool externalLinkage)
{
    assert(param.size() == fnType->paramType().size());

    forgetAllLocalVariables();
    auto fn = functionDeclaration(ident, fnType, externalLinkage);
    fn->setDoesNotThrow();
    llvmBB = llvm::BasicBlock::Create(*llvmContext, "entry", fn);
    llvmBuilder->SetInsertPoint(llvmBB);

    auto retType = fnType->retType();

    // main by default returns int:
    functionBuildingInfo.isMain = !strcmp(ident, "main");
    if (functionBuildingInfo.isMain && retType->isVoid()) {
	retType = abc::IntegerType::createInt();
    }

    functionBuildingInfo.fn = fn;
    functionBuildingInfo.leave = getLabel(".leave");
    functionBuildingInfo.retType = retType;
    functionBuildingInfo.retVal = nullptr;
    functionBuildingInfo.bbClosed = false;

    // reconstruct arguments
    for (std::size_t i = 0; i < param.size(); ++i) {
	// std::cerr << ">> i = " << i << "\n";
	auto addr = localVariableDefinition(param[i], fnType->paramType()[i]);
	auto argInfo = abi::classifyArgType(fnType->paramType()[i]);
	if (argInfo.byVal) {
	    auto tmp = fetch(fn->getArg(i), argInfo.byValType);
	    store(tmp, addr, fnType->paramType()[i]);
	} else {
	    store(fn->getArg(i), addr, argInfo.type);
	}
    }

    if (!retType->isVoid()) {
	functionBuildingInfo.retVal =
	    localVariableDefinition(".retVal", retType);
	if (functionBuildingInfo.isMain) {
	    store(getConstantInt("0", retType), functionBuildingInfo.retVal,
	          retType);
	} else {
	    auto llvmRetType = convert(retType);
	    store(llvm::UndefValue::get(llvmRetType),
	          functionBuildingInfo.retVal, retType);
	}
    }
}

bool
functionDefinitionEnd()
{
    assert(llvmBuilder);
    assert(functionBuildingInfo.fn);
    assert(functionBuildingInfo.leave);

    bool wellFormed = true;

    auto checkReturn = getLabel("checkReturn");
    if (!functionBuildingInfo.bbClosed) {
	jumpInstruction(checkReturn);
    }

    llvm::EliminateUnreachableBlocks(*functionBuildingInfo.fn);

    /*
    // https://stackoverflow.com/a/53634733/909565
    for (auto &bb : *functionBuildingInfo.fn) {
        auto *terminator = bb.getTerminator();
        if (terminator != nullptr) {
            continue; // Well-formed
        }
        if (functionBuildingInfo.fn->getReturnType()->isVoidTy()) {
            continue; // Ok, return gets generated below
        }
        wellFormed = false;
        llvmBuilder->CreateUnreachable();
    }
    */

    defineLabel(checkReturn);
    if (checkReturn->hasNPredecessorsOrMore(1)) {
	wellFormed = functionBuildingInfo.isMain ||
	             functionBuildingInfo.fn->getReturnType()->isVoidTy();
	if (!wellFormed) {
	    llvmBuilder->CreateUnreachable();
	}
    }
    jumpInstruction(functionBuildingInfo.leave);
    llvm::EliminateUnreachableBlocks(*functionBuildingInfo.fn);

    defineLabel(functionBuildingInfo.leave);
    if (functionBuildingInfo.retType->isVoid()) {
	llvmBuilder->CreateRetVoid();
    } else {
	auto retVal =
	    fetch(functionBuildingInfo.retVal, functionBuildingInfo.retType);
	llvmBuilder->CreateRet(retVal);
    }

    llvm::verifyFunction(*functionBuildingInfo.fn);

    functionBuildingInfo.fn = nullptr;
    functionBuildingInfo.leave = nullptr;
    functionBuildingInfo.retType = nullptr;
    functionBuildingInfo.retVal = nullptr;
    functionBuildingInfo.bbClosed = true;
    functionBuildingInfo.isMain = false;
    return wellFormed;
}

Value
functionCall(Value fnAddr, const abc::Type *fnType,
             const std::vector<Value> &arg)
{
    assert(fnType);
    auto llvmFnType = llvm::dyn_cast<llvm::FunctionType>(convert(fnType));
    assert(llvmFnType);
    auto fnCall = llvmBuilder->CreateCall(llvmFnType, fnAddr, arg);

    auto abcParamType = fnType->paramType();
    // lower arguments
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.byVal) {
	    fnCall->addParamAttr(i,
	                         llvm::Attribute::getWithByValType(
	                             *llvmContext, convert(argInfo.byValType)));
	    fnCall->addParamAttr(i, llvm::Attribute::getWithAlignment(
	                                *llvmContext, argInfo.align));
	}
    }
    return fnCall;
}

} // namespace gen
