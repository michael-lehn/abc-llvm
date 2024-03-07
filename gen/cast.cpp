#include <iostream>

#include "cast.hpp"
#include "gentype.hpp"

namespace gen {

Value
cast(Value val, const abc::Type *fromType, const abc::Type *toType)
{
    assert(llvmContext);
    assert(fromType);
    assert(toType);

    fromType = fromType->getConstRemoved();
    toType = toType->getConstRemoved();

    if (fromType == toType) {
	return val;
    }

    auto llvmToType = convert(toType);

    if (fromType->isInteger() && toType->isInteger()) {
	if (toType->numBits() > fromType->numBits()) {
	    return fromType->isUnsignedInteger()
		? llvmBuilder->CreateZExtOrBitCast(val, llvmToType)
		: llvmBuilder->CreateSExtOrBitCast(val, llvmToType); 
	} else {
	    return llvmBuilder->CreateTruncOrBitCast(val, llvmToType); 
	}
    } else if (fromType->isPointer() && toType->isPointer()) {
	return val;
    }
    std::cerr << "gen::cat: can not cast '" << fromType << "' to '" << toType
	<< "'\n";
    assert(0);
    return nullptr;
}

Constant
cast(Constant val, const abc::Type *fromType, const abc::Type *toType)
{
    assert(llvmContext);
    auto val_ = llvm::dyn_cast<llvm::Value>(val);
    assert(val_);
    return llvm::dyn_cast<llvm::Constant>(cast(val_, fromType, toType));
}

} // namespace gen

