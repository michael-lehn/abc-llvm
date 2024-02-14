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
using ConstIntVal = llvm::ConstantInt *;

// enable/disable optimization
void setOpt(bool);
void setTarget(int codeGenOptLevel);

// functions definition
void fnDecl(const char *ident, const Type *fn);
void fnDef(const char *ident, const Type *fn,
	   const std::vector<const char *> &param);
void fnDefEnd(void);
void ret(Reg reg = nullptr);

// variables
void defLocal(const char *ident, const Type *type);
void defGlobal(const char *ident, const Type *type, bool external,
	       ConstVal val = nullptr);
void declGlobal(const char *ident, const Type *type);
void defStatic(const char *ident, const Type *type, ConstVal constVal);
void defStringLiteral(const char *ident, const char *val, bool isConst);

// size of type
std::size_t getSizeOf(const Type *type);

// functions call
Reg call(const char *ident, const std::vector<Reg> &param);
Reg call(Reg fnPtr, const Type *fnType, const std::vector<Reg> &param);

// check if generated instructions are reachabel
bool openBuildingBlock();

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
ConstVal cond(CondOp op, ConstVal a, ConstVal b);

Label getLabel(const char *name = "");
void labelDef(Label label);
Label jmp(Label &label); // needed for phi: returns label of current block
void jmp(Cond cond, Label trueLabel, Label falseLabel);
void jmp(Cond cond, Label defaultLabel,
	 const std::vector<std::pair<ConstIntVal, Label>> &caseLabel);
Reg phi(Reg a, Label labelA, Reg b, Label labelB, const Type *type);

// memory
Reg loadAddr(const char *ident);
Reg fetch(const char *ident, const Type *type);
Reg fetch(Reg addr, const Type *type);
Reg store(Reg val, const char *ident, const Type *type);
Reg store(Reg val, Reg addr, const Type *type);

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
ConstVal cast(ConstVal constVal, const Type *fromType, const Type *toType);

ConstVal loadIntConst(const char *val, const Type *type, std::uint8_t radix);
ConstVal loadIntConst(std::uint64_t val, const Type *type,
		      bool isSigned = false);
ConstVal loadZero(const Type *type);

ConstVal loadConstString(const char *str);
ConstVal loadConstArray(const std::vector<ConstVal> &val, const Type *type);
ConstVal loadConstStruct(const std::vector<ConstVal> &val, const Type *type);

//Reg loadConst(const ConstExpr *constExpr);

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
