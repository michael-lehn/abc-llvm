#include <iostream>

#include "function.hpp"
#include "instruction.hpp"
#include "variable.hpp"

namespace gen {

Value
instruction(AluOp op, Value left, Value right)
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
	default:
	    assert(0);
	    return nullptr;
    }
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

void
returnInstruction(Value val)
{
    if (val) {
	store(val, functionBuildingInfo.retVal);
    }
    jumpInstruction(functionBuildingInfo.leave);
}

} // namespace gen
