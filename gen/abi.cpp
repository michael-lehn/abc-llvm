#include <iostream>
#include <optional>

#include "abi.hpp"
#include "gentype.hpp"
#include "type/floattype.hpp"
#include "type/integertype.hpp"
#include "type/pointertype.hpp"
#include "variable.hpp"

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

static bool
isHomogeneousAggregate(const abc::Type *abcType, const abc::Type *elementType,
                       std::size_t size)
{
    if (!abcType->isStruct() && !abcType->isArray()) {
	return false;
    }
    if (abcType->aggregateSize() != size) {
	return false;
    }
    if (abcType->isArray() &&
        abc::Type::equals(abcType->refType(), elementType)) {
	return true;
    }
    assert(abcType->isStruct());
    for (std::size_t i = 0; i < size; ++i) {
	if (!abc::Type::equals(abcType->aggregateType(i), elementType)) {
	    return false;
	}
    }
    return true;
}

std::optional<std::vector<llvm::Type *>>
lowerType(const abc::Type *abcType)
{
    static const abc::Type *abcFloatTy = abc::FloatType::createFloat();
    static llvm::Type *llvmFloatTy = llvm::Type::getFloatTy(*llvmContext);

    static const abc::Type *abcU8Ty = abc::IntegerType::createUnsigned(8);

    using VecTy = std::vector<llvm::Type *>;

    if (isHomogeneousAggregate(abcType, abcFloatTy, 2)) {
	return VecTy{llvm::FixedVectorType::get(llvmFloatTy, 2)};
    }
    if (isHomogeneousAggregate(abcType, abcFloatTy, 4)) {
	return VecTy{
	    llvm::FixedVectorType::get(llvmFloatTy, 2),
	    llvm::FixedVectorType::get(llvmFloatTy, 2),
	};
    }
    if (isHomogeneousAggregate(abcType, abcU8Ty, 4)) {
	return VecTy{llvm::Type::getInt32Ty(*llvmContext)};
    }
    return std::nullopt;
}

ArgInfo
classifyArgType(const abc::Type *abcType)
{
    ArgInfo argInfo;

    argInfo.abcType = abcType;
    argInfo.align = getAlignof(abcType);

    if (auto abiType = lowerType(abcType)) {
	argInfo.kind = ArgInfo::Kind::Coerce;
	argInfo.abiType = std::move(*abiType);
    } else if (abcType->isStruct() || abcType->isArray()) {
	argInfo.kind = ArgInfo::Kind::ByVal;
	argInfo.abiType.push_back(convert(abc::PointerType::create(abcType)));
    } else {
	argInfo.kind = ArgInfo::Kind::Direct;
	argInfo.abiType.push_back(convert(abcType));
    }

    return argInfo;
}

// lower function declaration
llvm::FunctionType *
lowerFunctionType(const abc::Type *abcFnType)
{
    auto abcParamType = abcFnType->paramType();
    std::vector<llvm::Type *> paramType;

    for (auto *abcType : abcFnType->paramType()) {
	auto argInfo = classifyArgType(abcType);
	for (auto *ty : argInfo.abiType) {
	    paramType.push_back(ty);
	}
    }
    return llvm::FunctionType::get(convert(abcFnType->retType()), paramType,
                                   abcFnType->hasVarg());
}

llvm::Function *
lowerFunctionDeclaration(const char *ident, const abc::Type *abcFnType,
                         bool externalLinkage)
{
    assert(llvmContext);
    if (auto fn = llvmModule->getFunction(ident)) {
	// already declared
	/*
	 * Whether a function is external or not is specified by its
	 * first declaration. Like in C:
	 * - An extern declaration can be followed by a static declaration
	 * - A static static declaration *can not* be followed by an extern
	 *   declaration.
	 */
	assert(!externalLinkage ||
	       fn->getLinkage() == llvm::Function::ExternalLinkage);
	return fn;
    }

    auto linkage = externalLinkage || !strcmp(ident, "main")
                       ? llvm::Function::ExternalLinkage
                       : llvm::Function::InternalLinkage;

    auto llvmFnType = lowerFunctionType(abcFnType);

    auto fn =
        llvm::Function::Create(llvmFnType, linkage, ident, llvmModule.get());

    // lower declaration
    std::size_t abiIndex = 0;
    for (auto *abcType : abcFnType->paramType()) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcType);
	if (argInfo.kind == ArgInfo::Kind::ByVal) {
	    fn->addParamAttr(abiIndex,
	                     llvm::Attribute::getWithByValType(
	                         *llvmContext, convert(argInfo.abcType)));
	    fn->addParamAttr(abiIndex, llvm::Attribute::getWithAlignment(
	                                   *llvmContext, argInfo.align));
	}
	abiIndex += argInfo.abiType.size();
    }

    return fn;
}

static void
lowerArgument(Value arg, const ArgInfo &argInfo, std::vector<Value> &abiArg)
{
    if (argInfo.kind == ArgInfo::Kind::Coerce) {
	std::size_t offset = 0;
	for (auto *ty : argInfo.abiType) {
	    auto *p = llvmBuilder->CreateConstGEP1_64(
	        llvm::Type::getInt8Ty(*llvmContext), arg, offset);
	    abiArg.push_back(llvmBuilder->CreateLoad(ty, p));
	    offset += 8;
	}
    } else {
	abiArg.push_back(arg);
    }
}

Value
lowerFunctionCall(Value fnAddr, const abc::Type *fnAbcType,
                  const std::vector<Value> &arg)
{
    assert(fnAbcType);
    auto llvmFnType = lowerFunctionType(fnAbcType);
    assert(llvmFnType);

    auto abcParamType = fnAbcType->paramType();

    std::vector<Value> abiArg;
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	auto argInfo = classifyArgType(abcParamType[i]);
	lowerArgument(arg[i], argInfo, abiArg);
    }
    for (std::size_t i = abcParamType.size(); i < arg.size(); ++i) {
	abiArg.push_back(arg[i]);
    }

    auto fnCall = llvmBuilder->CreateCall(llvmFnType, fnAddr, abiArg);

    std::size_t abiIndex = 0;
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.kind == ArgInfo::Kind::ByVal) {
	    assert(argInfo.abiType.size() == 1);
	    fnCall->addParamAttr(abiIndex,
	                         llvm::Attribute::getWithByValType(
	                             *llvmContext, convert(argInfo.abcType)));
	    fnCall->addParamAttr(abiIndex, llvm::Attribute::getWithAlignment(
	                                       *llvmContext, argInfo.align));
	}
	abiIndex += argInfo.abiType.size();
    }
    return fnCall;
}

static void
reconstructArgument(const llvm::Function *fnDecl, std::size_t abiIndex,
                    Value addr, const ArgInfo &argInfo)
{
    if (argInfo.kind == ArgInfo::Kind::ByVal) {
	auto tmp =
	    fetch(fnDecl->getArg(abiIndex), argInfo.abiType[0], argInfo.align);

	store(tmp, addr, argInfo.abcType);
    } else if (argInfo.kind == ArgInfo::Kind::Coerce) {
	std::size_t offset = 0;

	for (std::size_t j = 0; j < argInfo.abiType.size(); ++j) {
	    auto *p = llvmBuilder->CreateConstGEP1_64(
	        llvm::Type::getInt8Ty(*llvmContext), addr, offset);

	    llvmBuilder->CreateStore(fnDecl->getArg(abiIndex + j), p);

	    offset += 8;
	}
    } else {
	assert(argInfo.kind == ArgInfo::Kind::Direct);
	store(fnDecl->getArg(abiIndex), addr, argInfo.abcType);
    }
}

void
reconstructParameters(const llvm::Function *fnDecl, const abc::Type *abcFnType,
                      const std::vector<const char *> &param)
{
    std::size_t abiIndex = 0;
    for (std::size_t i = 0; i < param.size(); ++i) {
	auto argInfo = abi::classifyArgType(abcFnType->paramType()[i]);
	auto addr =
	    localVariableDefinition(param[i], abcFnType->paramType()[i]);
	reconstructArgument(fnDecl, abiIndex, addr, argInfo);
	abiIndex += argInfo.abiType.size();

	/*
	if (argInfo.byVal) {
	    auto tmp = fetch(fnDecl->getArg(i), argInfo.abiType, argInfo.align);
	    store(tmp, addr, argInfo.abcType);
	} else {
	    store(fnDecl->getArg(i), addr, argInfo.abcType);
	}
	*/
    }
}

// lower arguments in function call

// reconstruct arguments in function

} // namespace abi
} // namespace gen
