#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "convert.hpp"
#include "gen.hpp"

#include "llvm/IR/DerivedTypes.h"

namespace gen {

static std::unordered_map<const abc::Type *, llvm::Type *> typeMap;

static std::vector<llvm::Type *>
    convert(const std::vector<const abc::Type *> &type);

llvm::Type *
convert(const abc::Type *abcType)
{
    if (typeMap.contains(abcType)) {
	return typeMap.at(abcType);
    }

    llvm::Type *llvmType = nullptr;


    if (abcType->isVoid()) {
	return llvm::Type::getVoidTy(*llvmContext);
    } else if (abcType->isInteger()) {
	switch (abcType->numBits()) {
	    case 1:
		llvmType = llvm::Type::getInt1Ty(*llvmContext);
		break;
	    case 8:
		llvmType = llvm::Type::getInt8Ty(*llvmContext);
		break;
	    case 16:
		llvmType = llvm::Type::getInt16Ty(*llvmContext);
		break;
	    case 32:
		llvmType = llvm::Type::getInt32Ty(*llvmContext);
		break;
	    case 64:
		llvmType = llvm::Type::getInt64Ty(*llvmContext);
		break;
	    default:
		llvmType = llvm::Type::getIntNTy(*llvmContext,
						 abcType->numBits());
		break;
	}
    } else if (abcType->isFunction()) {
	llvmType = llvm::FunctionType::get(convert(abcType->retType()),
					   convert(abcType->paramType()),
					   abcType->hasVarg());
    } else if (abcType->isPointer()) {
	llvmType = llvm::PointerType::get(*llvmContext, 0);
    } else if (abcType->isArray()) {
	llvmType = llvm::ArrayType::get(convert(abcType->refType()),
					abcType->dim());
    } else if (abcType->isStruct()) {
	auto abcMemberType = abcType->memberType();
	std::vector<llvm::Type *> llvmMemberType{abcMemberType.size()};
	for (std::size_t i = 0; i < abcMemberType.size(); ++i) {
	    llvmMemberType[i] = convert(abcMemberType[i]);
	}
	llvmType = llvm::StructType::get(*llvmContext, llvmMemberType);
    } else {
	assert(0);
	return nullptr;
    }
    typeMap[abcType] = llvmType;
    return llvmType;
}

static std::vector<llvm::Type *>
convert(const std::vector<const abc::Type *> &abcType)
{
    std::vector<llvm::Type *> llvmType{abcType.size()};
    for (std::size_t i = 0; i < abcType.size(); ++i) {
	llvmType[i] = convert(abcType[i]);
    }
    return llvmType;
}

} // namespace gen
