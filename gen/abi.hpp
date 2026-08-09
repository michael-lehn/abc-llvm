#ifndef GEN_ABI_HPP
#define GEN_ABI_HPP

#include <vector>

#include "gen.hpp"
#include "type/type.hpp"

namespace gen {
namespace abi {

bool needsAddress(const abc::Type *argType);

//------------------------------------------------------------------------------

struct ArgInfo
{
	const abc::Type *abcType;

	bool byVal;
	bool lowered;
	llvm::Type *abiType;
	llvm::Align align;
};

ArgInfo classifyArgType(const abc::Type *abcType);

llvm::FunctionType *lowerFunctionType(const abc::Type *abcFnType);

llvm::Function *lowerFunctionDeclaration(const char *ident,
                                         const abc::Type *abcFnType,
                                         bool externalLinkage);

Value lowerFunctionCall(Value fnAddr, const abc::Type *fnType,
                        const std::vector<Value> &arg);

void reconstructParameters(const llvm::Function *fnDecl,
                           const abc::Type *abcFnType,
                           const std::vector<const char *> &param);

} // namespace abi
} // namespace gen

#endif // GEN_CAST_HPP
