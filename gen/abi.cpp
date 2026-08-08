#include "abi.hpp"
#include "gentype.hpp"
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
llvm::FunctionType *
lowerFunctionType(const abc::Type *abcFnType)
{
    auto abcParamType = abcFnType->paramType();
    std::vector<llvm::Type *> paramType;
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	auto argInfo = classifyArgType(abcParamType[i]);
	paramType.push_back(convert(argInfo.type));
    }
    return llvm::FunctionType::get(convert(abcFnType->retType()), paramType,
                                   abcFnType->hasVarg());
}

Value
lowerFunctionCall(Value fnAddr, const abc::Type *fnAbcType,
                  const std::vector<Value> &arg)
{
    assert(fnAbcType);
    auto llvmFnType = lowerFunctionType(fnAbcType);
    assert(llvmFnType);
    auto fnCall = llvmBuilder->CreateCall(llvmFnType, fnAddr, arg);

    auto abcParamType = fnAbcType->paramType();
    // lower arguments
    for (std::size_t i = 0; i < abcParamType.size(); ++i) {
	abi::ArgInfo argInfo = abi::classifyArgType(abcParamType[i]);
	if (argInfo.byVal) {
	    fnCall->addParamAttr(i,
	                         llvm::Attribute::getWithByValType(
	                             *llvmContext, convert(argInfo.byValType)));
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
	    auto tmp = fetch(fnDecl->getArg(i), argInfo.byValType);
	    store(tmp, addr, abcFnType->paramType()[i]);
	} else {
	    store(fnDecl->getArg(i), addr, argInfo.type);
	}
    }
}

// lower arguments in function call

// reconstruct arguments in function

} // namespace abi
} // namespace gen
