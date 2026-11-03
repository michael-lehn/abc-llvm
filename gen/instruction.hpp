#ifndef GEN_INSTRUCTION_HPP
#define GEN_INSTRUCTION_HPP

#include "type/type.hpp"

#include "gen.hpp"

namespace gen {

enum InstructionOp
{
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FEQ,
    FNE, // not equal or unordered: true for NaN, as in C
    FGT,
    FGE,
    FLT,
    FLE,

    ADD,
    SUB,
    SMUL,
    SDIV,
    SMOD,
    UDIV,
    UMOD,
    EQ,
    NE,
    SGT,
    SGE,
    SLT,
    SLE,
    UGT,
    UGE,
    ULT,
    ULE,
    AND,
    OR,
    XOR,
    SHL,  // shift left
    LSHR, // logical shift right
    ASHR, // arithmetic shift right
};

// Instruction for comparing a value used as condition with zero. As in C every
// value but zero is true, including NaN.
InstructionOp conditionOp(const abc::Type *type);

Value instruction(InstructionOp op, Value left, Value right);
Constant instruction(InstructionOp op, Constant left, Constant right);

JumpOrigin jumpInstruction(Label label);
JumpOrigin jumpInstruction(Value condition, Label trueLabel, Label falseLabel);

using CaseLabel = std::pair<ConstantInt, Label>;
JumpOrigin jumpInstruction(Value condition, Label defaultLabel,
                           const std::vector<CaseLabel> &caseLabel);

Value phi(Value a, Label labelA, Value b, Label labelB, const abc::Type *type);

void returnInstruction(Value val, const abc::Type *retType);
void reachableCheck();

} // namespace gen

#endif // GEN_INSTRUCTION_HPP
