#include <iostream>

#include "constant.hpp"
#include "convert.hpp"
#include "gen.hpp"

namespace gen {

ConstantInt
getConstantInt(const char *val, const abc::Type *type, std::uint8_t radix)
{
    assert(llvmContext);
    assert(type);
    assert(val);
    assert(type->isInteger());

    auto apint = llvm::APInt(type->numBits(), val, radix);
    return llvm::ConstantInt::get(*llvmContext, apint);
}

Constant
getConstantZero(const abc::Type *type)
{
    assert(llvmContext);
    assert(type);

    auto llvmType = convert(type);
    return llvm::Constant::getNullValue(llvmType);
}

} // namespace gen
