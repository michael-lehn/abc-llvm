#include <cassert>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "abi.hpp"
#include "gen.hpp"
#include "gentype.hpp"

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/DerivedTypes.h"

namespace gen {

static std::unordered_map<const abc::Type *, llvm::Type *> typeMap;

void
initTypeMap()
{
    typeMap.clear();
}

llvm::Type *
convert(const abc::Type *abcType)
{
    if (typeMap.contains(abcType)) {
	return typeMap.at(abcType);
    }

    llvm::Type *llvmType = nullptr;

    if (abcType->isVoid()) {
	llvmType = llvm::Type::getVoidTy(*llvmContext);
    } else if (abcType->isFloat()) {
	llvmType = llvm::Type::getFloatTy(*llvmContext);
    } else if (abcType->isDouble()) {
	llvmType = llvm::Type::getDoubleTy(*llvmContext);
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
	    llvmType = llvm::Type::getIntNTy(*llvmContext, abcType->numBits());
	    break;
	}
    } else if (abcType->isFunction()) {
	llvmType = abi::lowerFunctionType(abcType);
    } else if (abcType->isPointer()) {
	llvmType = llvm::PointerType::get(*llvmContext, 0);
    } else if (abcType->isArray()) {
	llvmType =
	    llvm::ArrayType::get(convert(abcType->refType()), abcType->dim());
    } else if (abcType->isStruct()) {
	auto abcMemberType = abcType->memberType();
	auto abcMemberIndex = abcType->memberIndex();
	auto lastIndex = abcMemberIndex.back();
	std::vector<llvm::Type *> llvmMemberType{lastIndex + 1};
	for (std::size_t i = 0, pos = 0; i <= lastIndex; ++i) {
	    std::size_t maxSize = 0;
	    while (pos < abcMemberIndex.size() && abcMemberIndex[pos] == i) {
		if (getSizeof(abcMemberType[pos]) > maxSize) {
		    maxSize = getSizeof(abcMemberType[pos]);
		    llvmMemberType[i] = convert(abcMemberType[pos]);
		}
		++pos;
	    }
	}
	llvmType = llvm::StructType::get(*llvmContext, llvmMemberType);
    } else {
	std::cerr << "gen::convert with type '" << abcType << "'\n";
	assert(0);
	return nullptr;
    }
    typeMap[abcType] = llvmType;
    return llvmType;
}

std::size_t
getSizeof(const abc::Type *type)
{
    assert(llvmContext && "gen::init called?");
    auto llvmType = convert(type);
    return llvmModule->getDataLayout().getTypeAllocSize(llvmType);
}

llvm::Align
getAlignof(const abc::Type *type)
{
    assert(llvmContext && "gen::init called?");
    auto llvmType = convert(type);
    return llvmModule->getDataLayout().getPrefTypeAlign(llvmType);
}

} // namespace gen
