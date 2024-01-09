#ifndef GEN_HPP
#define GEN_HPP

#include <vector>

#include "llvm/IR/Value.h"

#include "type.hpp"

namespace gen {

using Cond = llvm::Value *;
using Label = llvm::BasicBlock *;
using Reg = llvm::Value *;
using ConstVal = llvm::Constant *;

// enable/disable optimization
void setOpt(bool);

// functions definition
void fnDecl(const char *ident, const Type *fn);
void fnDef(const char *ident, const Type *fn,
	   const std::vector<const char *> &param);
void fnDefEnd(void);
void ret(Reg reg = nullptr);

// variables
void defLocal(const char *ident, const Type *type);
void defGlobal(const char *ident, const Type *type, ConstVal constVal);
void defStatic(const char *ident, const Type *type, ConstVal constVal);

// functions call
Reg call(const char *ident, const std::vector<Reg> &param);

// labels and jumps
enum CondOp {
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
};

Cond cond(CondOp op, Reg a, Reg b);
Label getLabel(const char *name = "");
void labelDef(Label label);
void jmp(Label label);
void jmp(Cond cond, Label trueLabel, Label falseLabel);
Reg phi(Reg a, Label labelA, Reg b, Label labelB, const Type *type);

// memory 
Reg fetch(const char *ident, const Type *type);
void store(Reg reg, const char *ident, const Type *type);

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

Reg loadConst(const char *val, const Type *type);
Reg aluInstr(AluOp op, Reg l, Reg r);

void dump_bc(const char *filename = "out");
void dump_asm(const char *filename = "out", int codeGenOptLevel = 0);

} // namespace gen

#endif // GEN_HPP
