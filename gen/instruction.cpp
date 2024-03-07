#include <iostream>

#include "function.hpp"
#include "gentype.hpp"
#include "instruction.hpp"
#include "variable.hpp"

namespace gen {

Value
instruction(InstructionOp op, Value left, Value right)
{
    assert(llvmContext);

    switch (op) {
	case ADD:
	    return llvmBuilder->CreateAdd(left, right);
	case SUB:
	    return llvmBuilder->CreateSub(left, right);
	case SMUL:
	    return llvmBuilder->CreateMul(left, right);
	case SDIV:
	    return llvmBuilder->CreateSDiv(left, right);
	case SMOD:
	    return llvmBuilder->CreateSRem(left, right);
	case UDIV:
	    return llvmBuilder->CreateUDiv(left, right);
	case UMOD:
	    return llvmBuilder->CreateURem(left, right);
	case EQ:
	    return llvmBuilder->CreateICmpEQ(left, right);
	case NE:
	    return llvmBuilder->CreateICmpNE(left, right);
	case SGT:
	    return llvmBuilder->CreateICmpSGT(left, right);
    	case SGE:
	    return llvmBuilder->CreateICmpSGE(left, right);
    	case SLT:
	    return llvmBuilder->CreateICmpSLT(left, right);
    	case SLE:
	    return llvmBuilder->CreateICmpSLE(left, right);
    	case UGT:
	    return llvmBuilder->CreateICmpUGT(left, right);
    	case UGE:
	    return llvmBuilder->CreateICmpUGE(left, right);
    	case ULT:
	    return llvmBuilder->CreateICmpULT(left, right);
    	case ULE:
	    return llvmBuilder->CreateICmpULE(left, right);
    	case AND:
	    return llvmBuilder->CreateAnd(left, right);
    	case OR:
	    return llvmBuilder->CreateOr(left, right);
	default:
	    assert(0);
	    return nullptr;
    }
}

Constant
instruction(InstructionOp op, Constant left, Constant right)
{
    auto left_ = llvm::dyn_cast<llvm::Value>(left);
    auto right_ = llvm::dyn_cast<llvm::Value>(right);
    return llvm::dyn_cast<llvm::Constant>(instruction(op, left_, right_));
}

JumpOrigin
jumpInstruction(Label label)
{
    assert(llvmBuilder);
    assert(functionBuildingInfo.fn);
    assert(!functionBuildingInfo.bbClosed);

    auto ib = llvmBuilder->GetInsertBlock();
    llvmBuilder->CreateBr(label);
    functionBuildingInfo.bbClosed = true;
    return ib;
}

JumpOrigin
jumpInstruction(Value condition, Label trueLabel, Label falseLabel)
{
    assert(llvmBuilder);
    assert(functionBuildingInfo.fn);
    assert(!functionBuildingInfo.bbClosed);

    auto ib = llvmBuilder->GetInsertBlock();
    llvmBuilder->CreateCondBr(condition, trueLabel, falseLabel);
    functionBuildingInfo.bbClosed = true;
    return ib;
}

Value
phi(Value a, Label labelA, Value b, Label labelB, const abc::Type *type)
{
    assert(llvmBuilder);

    auto llvmType = convert(type);
    auto phi = llvmBuilder->CreatePHI(llvmType, 2);
    phi->addIncoming(a, labelA);
    phi->addIncoming(b, labelB);
    return phi;
}

void
returnInstruction(Value val)
{
    assert(llvmBuilder);
    assert(functionBuildingInfo.fn);
    assert(!functionBuildingInfo.bbClosed);

    if (val) {
	store(val, functionBuildingInfo.retVal);
    }
    jumpInstruction(functionBuildingInfo.leave);
}

} // namespace gen
