#ifndef GEN_FUNCTION
#define GEN_FUNCTION

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


llvm::Function *functionDeclaration(const char *ident, const abc::Type *fnType,
				    bool externalLinkage);

void functionDefinitionBegin(const char *ident, const abc::Type *fnType,
			     const std::vector<const char *> &arg,
			     bool externalLinkage);

void functionDefinitionEnd();

Value functionCall(Value fnAddr, const abc::Type *fnType,
		   const std::vector<Value> &arg);

} // namespace gen

#endif // GEN_FUNCTION
