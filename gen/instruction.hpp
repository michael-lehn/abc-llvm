#ifndef GEN_INSTRUCTION_HPP
#define GEN_INSTRUCTION_HPP

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

JumpOrigin jumpInstruction(Label label);

JumpOrigin jumpInstruction(Value condition, Label trueLabel, Label falseLabel);

void returnInstruction(Value val);

} // namespace gen

#endif // GEN_INSTRUCTION_HPP
