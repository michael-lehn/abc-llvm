#ifndef GEN_HPP
#define GEN_HPP

#include <cstdint>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"

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
void defGlobal(const char *ident, const Type *type, ConstVal val = nullptr);
void defStatic(const char *ident, const Type *type, ConstVal constVal);
void defStringLiteral(const char *ident, const char *val, bool isConst);

// functions call
Reg call(const char *ident, const std::vector<Reg> &param);
Reg call(Reg fnPtr, const Type *fnType, const std::vector<Reg> &param);

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
    AND,
    OR,
};

Cond cond(CondOp op, Reg a, Reg b);
Label getLabel(const char *name = "");
void labelDef(Label label);
Label jmp(Label &label); // needed for phi: returns label of current block
void jmp(Cond cond, Label trueLabel, Label falseLabel);
Reg phi(Reg a, Label labelA, Reg b, Label labelB, const Type *type);

// memory
Reg loadAddr(const char *ident);
Reg fetch(const char *ident, const Type *type);
Reg fetch(Reg addr, const Type *type);
void store(Reg val, const char *ident, const Type *type);
void store(Reg val, Reg addr, const Type *type);

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

Reg cast(Reg reg, const Type *fromType, const Type *toType);
Reg loadIntConst(const char *val, const Type *type, std::uint8_t radix);
Reg loadIntConst(std::uint64_t val, const Type *type);
Reg aluInstr(AluOp op, Reg l, Reg r);

// compute addr + offset
Reg ptrInc(const Type *type, Reg addr, Reg offset);
Reg ptrMember(const Type *type, Reg addr, std::size_t index);
// compute addrLeft - addrRight
Reg ptrDiff(const Type *type, Reg addrLeft, Reg addRight);

void dump_bc(const char *filename = "out");
void dump_asm(const char *filename = "out", int codeGenOptLevel = 0);

} // namespace gen

#endif // GEN_HPP
