#ifndef GEN_FUNCTION
#define GEN_FUNCTION

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/Function.h"

#include "type/type.hpp"

#include "gen.hpp"

namespace gen {

struct FunctionBuildingInfo
{
    llvm::Function *fn = nullptr;
    Label leave = nullptr;
    const abc::Type *retType = nullptr;
    Value retVal = nullptr;
    bool bbClosed = true;
};

extern FunctionBuildingInfo functionBuildingInfo;

// allows to check if we are in a building block. Otherwise instructions are
// not reachable
bool bbOpen();


llvm::Function *functionDeclaration(const char *ident, const abc::Type *fnType,
				    bool externalLinkage);

void functionDefinitionBegin(const char *ident, const abc::Type *fnType,
			     const std::vector<const char *> &arg,
			     bool externalLinkage);

bool functionDefinitionEnd();

Value functionCall(Value fnAddr, const abc::Type *fnType,
		   const std::vector<Value> &arg);

} // namespace gen

#endif // GEN_FUNCTION
