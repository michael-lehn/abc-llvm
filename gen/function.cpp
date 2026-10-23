#include <cstring>
#include <iostream>
#include <unordered_set>
#include <vector>

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

// Removes basic blocks that are not reachable from the entry block. Like
// llvm::EliminateUnreachableBlocks() but jump targets may be labels that are
// not defined yet, i.e. blocks without terminator (since LLVM 23 successors()
// requires a terminator).
//
// Precondition: every block that is already inserted into the function has a
// terminator (llvm::DeleteDeadBlocks() needs the successors of dead blocks).
// This holds in functionDefinitionEnd(): defineLabel() closes the current
// block before it inserts a label, and the current block is closed before
// this function is called.
static void
eliminateUnreachableBlocks(llvm::Function &fn)
{
    std::unordered_set<llvm::BasicBlock *> reachable;
    std::vector<llvm::BasicBlock *> pending{&fn.getEntryBlock()};
    while (!pending.empty()) {
	auto bb = pending.back();
	pending.pop_back();
	if (!reachable.insert(bb).second) {
	    continue;
	}
	if (bb->empty() || !bb->back().isTerminator()) {
	    continue;
	}
	auto &terminator = bb->back();
	for (unsigned i = 0; i < terminator.getNumSuccessors(); ++i) {
	    pending.push_back(terminator.getSuccessor(i));
	}
    }

    std::vector<llvm::BasicBlock *> dead;
    for (auto &bb : fn) {
	if (!reachable.contains(&bb)) {
	    assert(!bb.empty() && bb.back().isTerminator());
	    dead.push_back(&bb);
	}
    }
    llvm::DeleteDeadBlocks(dead);
}

bool
bbOpen()
{
    return !functionBuildingInfo.bbClosed;
}

llvm::Function *
functionDeclaration(const char *ident, const abc::Type *fnType,
                    bool externalLinkage)
{
    return abi::lowerFunctionDeclaration(ident, fnType, externalLinkage);
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
    if (opt::sizeLevel >= 1) {
	fn->addFnAttr(llvm::Attribute::OptimizeForSize);
    }
    if (opt::sizeLevel >= 2) {
	fn->addFnAttr(llvm::Attribute::MinSize);
    }
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

    abi::reconstructParameters(fn, fnType, param);

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

    eliminateUnreachableBlocks(*functionBuildingInfo.fn);

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
    eliminateUnreachableBlocks(*functionBuildingInfo.fn);

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
    return abi::lowerFunctionCall(fnAddr, fnType, arg);
}

} // namespace gen
