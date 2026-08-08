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

void lowerArg(Value val, const abc::Type *abcType, Value &abiVal);

Value reconstructArg(const Value &abiVal, const abc::Type *abcType);

} // namespace abi
} // namespace gen

#endif // GEN_CAST_HPP
