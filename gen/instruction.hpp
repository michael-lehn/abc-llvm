#ifndef GEN_INSTRUCTION_HPP
#define GEN_INSTRUCTION_HPP

#include "type/type.hpp"

#include "constant.hpp"
#include "gen.hpp"

namespace gen {

enum InstructionOp {
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
};

Value instruction(InstructionOp op, Value left, Value right);
Constant instruction(InstructionOp op, Constant left, Constant right);

JumpOrigin jumpInstruction(Label label);
JumpOrigin jumpInstruction(Value condition, Label trueLabel, Label falseLabel);

Value phi(Value a, Label labelA, Value b, Label labelB, const abc::Type *type);

void returnInstruction(Value val);

} // namespace gen

#endif // GEN_INSTRUCTION_HPP
