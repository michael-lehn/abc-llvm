#include <iostream>

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

static llvm::Type *lowerType(const abc::Type *abcType);

bool
needsAddress(const abc::Type *argType)
{
    if (argType->isStruct() || argType->isArray()) {
	if (lowerType(argType)) {
	    return true;
	} else {
	    return true;
	}
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

static llvm::Type *
lowerType(const abc::Type *abcType)
{
    static const abc::Type *abcFloatTy = abc::FloatType::createFloat();
    static llvm::Type *llvmFloatTy = llvm::Type::getFloatTy(*llvmContext);

    static const abc::Type *abcU8Ty = abc::IntegerType::createUnsigned(8);

    if (isHomogeneousAggregate(abcType, abcFloatTy, 2)) {
	return llvm::FixedVectorType::get(llvmFloatTy, 2);
    }
    if (isHomogeneousAggregate(abcType, abcU8Ty, 4)) {
	return llvm::Type::getInt32Ty(*llvmContext);
    }
    return nullptr;
}

ArgInfo
classifyArgType(const abc::Type *abcType)
{
    ArgInfo argInfo;

    argInfo.abcType = abcType;
    argInfo.align = getAlignof(abcType);

    if ((argInfo.abiType = lowerType(abcType))) {
	argInfo.byVal = false;
	argInfo.lowered = true;
    } else if (abcType->isStruct() || abcType->isArray()) {
	argInfo.abiType = convert(abc::PointerType::create(abcType));
	argInfo.byVal = true;
	argInfo.lowered = false;
    } else {
	argInfo.abiType = convert(abcType);
	argInfo.byVal = false;
	argInfo.lowered = false;
    }

    return argInfo;
}

// lower function declaration
llvm::FunctionType *
lowerFunctionType(const abc::Type *abcFnType)
{
    auto abcParamType = abcFnType->paramType();
    std::vector<llvm::Type *> paramType;
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	auto argInfo = classifyArgType(abcParamType[i]);
	paramType.push_back(argInfo.abiType);
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
    auto abcParamType = abcFnType->paramType();
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.byVal) {
	    fn->addParamAttr(i, llvm::Attribute::getWithByValType(
	                            *llvmContext, convert(argInfo.abcType)));
	    fn->addParamAttr(i, llvm::Attribute::getWithAlignment(
	                            *llvmContext, argInfo.align));
	}
    }

    return fn;
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
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.lowered) {
	    abiArg.push_back(llvmBuilder->CreateLoad(argInfo.abiType, arg[i]));
	} else {
	    abiArg.push_back(arg[i]);
	}
    }
    for (std::size_t i = abcParamType.size(); i < arg.size(); ++i) {
	abiArg.push_back(arg[i]);
    }

    auto fnCall = llvmBuilder->CreateCall(llvmFnType, fnAddr, abiArg);

    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.byVal) {
	    fnCall->addParamAttr(
	        i, llvm::Attribute::getWithByValType(*llvmContext,
	                                             convert(argInfo.abcType)));
	    fnCall->addParamAttr(i, llvm::Attribute::getWithAlignment(
	                                *llvmContext, argInfo.align));
	}
    }
    return fnCall;
}

void
reconstructParameters(const llvm::Function *fnDecl, const abc::Type *abcFnType,
                      const std::vector<const char *> &param)
{
    for (std::size_t i = 0; i < param.size(); ++i) {
	// std::cerr << ">> i = " << i << "\n";
	auto addr =
	    localVariableDefinition(param[i], abcFnType->paramType()[i]);
	auto argInfo = abi::classifyArgType(abcFnType->paramType()[i]);
	if (argInfo.byVal) {
	    auto tmp = fetch(fnDecl->getArg(i), argInfo.abiType, argInfo.align);
	    store(tmp, addr, argInfo.abcType);
	} else {
	    store(fnDecl->getArg(i), addr, argInfo.abcType);
	}
    }
}

// lower arguments in function call

// reconstruct arguments in function

} // namespace abi
} // namespace gen
