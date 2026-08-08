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
	const abc::Type *type;

	bool byVal;
	const abc::Type *byValType;
	llvm::Align align;
};

ArgInfo classifyArgType(const abc::Type *abcType);

llvm::FunctionType *lowerFunctionType(const abc::Type *abcFnType);
Value lowerFunctionCall(Value fnAddr, const abc::Type *fnType,
                        const std::vector<Value> &arg);

void reconstructParameters(const llvm::Function *fnDecl,
                           const abc::Type *abcFnType,
                           const std::vector<const char *> &param);

} // namespace abi
} // namespace gen

#endif // GEN_CAST_HPP
