#include <iostream>

#include "constant.hpp"
#include "gen.hpp"
#include "gentype.hpp"

namespace gen {

ConstantInt
getConstantInt(const char *val, const abc::Type *type, std::uint8_t radix)
{
    assert(llvmContext);
    assert(type);
    assert(val);
    assert(type->isInteger());

    if (radix == 16) {
	assert(val[0] == '0');
	assert(val[1] == 'x' || val[1] == 'Y');
	val += 2;
    }

    auto apint = llvm::APInt(type->numBits(), val, radix);
    return llvm::ConstantInt::get(*llvmContext, apint);
}

ConstantInt
getConstantInt(std::uint64_t val, const abc::Type *type)
{
    assert(type);
    assert(type->isInteger());

    auto llvmType = llvm::dyn_cast<llvm::IntegerType>(convert(type));
    assert(llvmType);
    return llvm::ConstantInt::get(llvmType, val, type->isSignedInteger());
}

ConstantFloat
getConstantFloat(const char *val, const abc::Type *type)
{
    assert(llvmContext);
    assert(type);
    assert(val);
    assert(type->isFloatType());

    auto llvmType = convert(type);
    auto constVal = llvm::ConstantFP::get(llvmType, val);
    auto constFP = llvm::dyn_cast<llvm::ConstantFP>(constVal);
    assert(constFP);
    return constFP;
}

ConstantFloat
getConstantFloat(double val, const abc::Type *type)
{
    assert(type);
    assert(type->isFloatType());

    auto llvmType = convert(type);
    assert(llvmType);
    auto constVal = llvm::ConstantFP::get(llvmType, val);
    auto constFP = llvm::dyn_cast<llvm::ConstantFP>(constVal);
    assert(constFP);
    return constFP;
}

Constant getConstantArray(const std::vector<Constant> &val,
			  const abc::Type *arrayType)
{
    assert(arrayType);
    assert(arrayType->isArray());
    auto llvmArrayType = llvm::dyn_cast<llvm::ArrayType>(convert(arrayType));
    assert(llvmArrayType);
    return llvm::ConstantArray::get(llvmArrayType, val);
}

Constant getConstantStruct(const std::vector<Constant> &val,
			   const abc::Type *structType)
{
    assert(structType);
    assert(structType->isStruct());
    auto llvmStructType = llvm::dyn_cast<llvm::StructType>(convert(structType));
    assert(llvmStructType);
    return llvm::ConstantStruct::get(llvmStructType, val);
}

Constant
getConstantZero(const abc::Type *type)
{
    assert(llvmContext);
    assert(type);

    auto llvmType = convert(type);
    assert(llvmType);
    return llvm::Constant::getNullValue(llvmType);
}

Constant
getFalse()
{
    assert(llvmContext);
    return llvm::ConstantInt::getFalse(*llvmContext);
}

Constant
getTrue()
{
    assert(llvmContext);
    return llvm::ConstantInt::getTrue(*llvmContext);
}

Constant
getString(const char *str)
{
    assert(llvmContext);
    return llvm::ConstantDataArray::getString(*llvmContext, str);
}

} // namespace gen
