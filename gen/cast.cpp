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
    } else if (fromType->isFloatType() && toType->isFloatType()) {
	if (fromType->isFloat() && toType->isDouble()) {
	    return llvmBuilder->CreateFPExt(val, llvmToType);
	} else if (fromType->isDouble() && toType->isFloat()) {
	    return llvmBuilder->CreateFPTrunc(val, llvmToType);
	} else {
	    return val;
	}
    } else if (fromType->isInteger() && toType->isFloatType()) {
	return fromType->isUnsignedInteger()
	           ? llvmBuilder->CreateUIToFP(val, llvmToType)
	           : llvmBuilder->CreateSIToFP(val, llvmToType);
    } else if (fromType->isFloatType() && toType->isInteger()) {
	return toType->isUnsignedInteger()
	           ? llvmBuilder->CreateFPToUI(val, llvmToType)
	           : llvmBuilder->CreateFPToSI(val, llvmToType);
    } else if (fromType->isPointer() && toType->isPointer()) {
	return val;
    } else if (fromType->isArray() && toType->isArray()) {
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
