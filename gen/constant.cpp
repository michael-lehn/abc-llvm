#include <iostream>

#include "constant.hpp"
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

} // namespace gen
