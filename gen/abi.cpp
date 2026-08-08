#include "abi.hpp"
#include "gentype.hpp"
#include "type/pointertype.hpp"

namespace gen {
namespace abi {

/*
static bool
isX86_64Target()
{
assert(llvmModule);
return llvm::Triple{llvmModule->getTargetTriple()}.isX86_64();
}
*/

bool
needsAddress(const abc::Type *argType)
{
    if (argType->isStruct() || argType->isArray()) {
	return true;
    } else {
	return false;
    }
}

ArgInfo
classifyArgType(const abc::Type *abcType)
{
    ArgInfo argInfo;

    if (abcType->isStruct() || abcType->isArray()) {
	argInfo.type = abc::PointerType::create(abcType);
	argInfo.byVal = true;
	argInfo.byValType = abcType;
	argInfo.align = getAlignof(abcType);
    } else {
	argInfo.type = abcType;
	argInfo.byVal = false;
	argInfo.byValType = nullptr;
	argInfo.align = getAlignof(abcType);
    }

    return argInfo;
}

// lower function declaration

// lower arguments in function call

// reconstruct arguments in function

} // namespace abi
} // namespace gen
