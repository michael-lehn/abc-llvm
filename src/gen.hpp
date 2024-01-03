#ifndef GEN_HPP
#define GEN_HPP

#include <vector>

#include "llvm/IR/Value.h"

#include "types.hpp"

namespace gen {

void fnDecl(const char *ident, const Type *fn);
void fnDef(const char *ident, const Type *fn,
	   const std::vector<const char *> &param);



using Reg = llvm::Value;

// allocate local variable
void allocLocal(const char *ident, const Type *type);

// fetch and store
Reg *fetch(const char *ident, const Type *type);
void store(Reg *reg, const char *ident, const Type *type);

// load constant
Reg *loadConst(const char *val, const Type *type);

// ALU instructions

enum Op {
    Add,
    Sub,
    SMul,
    SDiv,
};

Reg *op2r(Op op, Reg *l, Reg *r);

// if reg is nullpoint return void, otherwise return value in *reg
void ret(Reg *reg = nullptr);

void dump_bc(const char *filename = "out");
void dump_asm(const char *filename = "out", int codeGenOptLevel = 0);

} // namespace gen

#endif // GEN_HPP
