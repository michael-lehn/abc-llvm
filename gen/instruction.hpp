#ifndef GEN_INSTRUCTION_HPP
#define GEN_INSTRUCTION_HPP

#include "gen.hpp"

namespace gen {

// ALU
enum AluOp {
    ADD,
    SUB,
    SMUL,
    SDIV,
    SMOD,
    UDIV,
    UMOD,
};

Value instruction(AluOp op, Value left, Value right);

JumpOrigin jumpInstruction(Label label);

void retInstruction(Value val);

} // namespace gen

#endif // GEN_INSTRUCTION_HPP
