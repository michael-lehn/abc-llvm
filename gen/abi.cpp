#include <algorithm>
#include <array>
#include <iostream>
#include <optional>

#include "abi.hpp"
#include "gentype.hpp"
#include "type/pointertype.hpp"
#include "variable.hpp"

namespace gen {
namespace abi {

static bool
isX86_64Target()
{
    assert(llvmModule);
    auto triple = llvm::Triple{llvmModule->getTargetTriple()};
#if LLVM_MAJOR_VERSION >= 22
    return triple.isX86_64();
#else
    return triple.getArch() == llvm::Triple::x86_64;
#endif
}

//------------------------------------------------------------------------------

bool
needsAddress(const abc::Type *argType)
{
    if (argType->isStruct() || argType->isArray()) {
	return true;
    } else {
	return false;
    }
}

// System V x86-64 ABI: an aggregate of at most 16 bytes is split into
// eightbytes. An eightbyte that only contains floating point values is passed
// in an SSE register, otherwise in an integer register.
struct Eightbyte
{
	bool used = false;
	bool isInteger = false;
	bool hasDouble = false;
	bool hasFloatAt[2] = {false, false};
};

using Eightbytes = std::array<Eightbyte, 2>;

static void
classifyEightbytes(const abc::Type *abcType, std::size_t offset, Eightbytes &eb)
{
    if (abcType->isStruct()) {
	auto llvmType = llvm::cast<llvm::StructType>(convert(abcType));
	auto layout = llvmModule->getDataLayout().getStructLayout(llvmType);
	const auto &memberType = abcType->memberType();
	const auto &memberIndex = abcType->memberIndex();
	for (std::size_t i = 0; i < memberType.size(); ++i) {
	    classifyEightbytes(memberType[i],
	                       offset + layout->getElementOffset(memberIndex[i]),
	                       eb);
	}
    } else if (abcType->isArray()) {
	auto elementSize = getSizeof(abcType->refType());
	for (std::size_t i = 0; i < abcType->dim(); ++i) {
	    classifyEightbytes(abcType->refType(), offset + i * elementSize, eb);
	}
    } else {
	auto &e = eb[offset / 8];
	e.used = true;
	if (abcType->isDouble()) {
	    e.hasDouble = true;
	} else if (abcType->isFloat()) {
	    e.hasFloatAt[offset % 8 / 4] = true;
	} else {
	    e.isInteger = true;
	}
    }
}

static std::optional<std::vector<llvm::Type *>>
lowerType_x86_64(const abc::Type *abcType)
{
    if (!abcType->isStruct() && !abcType->isArray()) {
	return std::nullopt;
    }
    auto size = getSizeof(abcType);
    if (size == 0 || size > 16) {
	return std::nullopt;
    }

    Eightbytes eb;
    classifyEightbytes(abcType, 0, eb);

    std::vector<llvm::Type *> abiType;
    for (std::size_t i = 0; i * 8 < size; ++i) {
	auto floatTy = llvm::Type::getFloatTy(*llvmContext);
	if (!eb[i].used || eb[i].isInteger) {
	    auto numBytes = std::min<std::size_t>(size - i * 8, 8);
	    abiType.push_back(
	        llvm::Type::getIntNTy(*llvmContext, numBytes * 8));
	} else if (eb[i].hasDouble) {
	    abiType.push_back(llvm::Type::getDoubleTy(*llvmContext));
	} else if (eb[i].hasFloatAt[1]) {
	    abiType.push_back(llvm::FixedVectorType::get(floatTy, 2));
	} else {
	    abiType.push_back(floatTy);
	}
    }
    return abiType;
}

std::optional<std::vector<llvm::Type *>>
lowerType(const abc::Type *abcType)
{
    if (isX86_64Target()) {
	return lowerType_x86_64(abcType);
    } else {
	return std::nullopt;
    }
}

static ArgInfo
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

// On x86-64 an aggregate that is not returned in registers is returned in
// memory provided by the caller (passed as hidden first argument). LLVM would
// otherwise return some of them in registers that C does not use.
static bool
returnsInMemory(const abc::Type *retType)
{
    return isX86_64Target() && (retType->isStruct() || retType->isArray()) &&
           !lowerType(retType);
}

static bool
isSseType(const llvm::Type *ty)
{
    return ty->isFloatingPointTy() || ty->isVectorTy();
}

// Classifies the parameters of a function. On x86-64 an aggregate gets passed
// in memory if there are not enough registers left for all of its eightbytes.
// An aggregate returned in memory uses an integer register for its address.
std::vector<ArgInfo>
classifyParamTypes(const abc::Type *abcFnType)
{
    std::size_t intRegs = 6, sseRegs = 8;
    if (returnsInMemory(abcFnType->retType())) {
	--intRegs;
    }

    std::vector<ArgInfo> argInfo;
    for (auto *abcType : abcFnType->paramType()) {
	auto info = classifyArgType(abcType);
	if (info.kind != ArgInfo::Kind::ByVal) {
	    std::size_t needSse = 0, needInt = 0;
	    for (auto *ty : info.abiType) {
		++(isSseType(ty) ? needSse : needInt);
	    }
	    if (needSse <= sseRegs && needInt <= intRegs) {
		sseRegs -= needSse;
		intRegs -= needInt;
	    } else if (info.kind == ArgInfo::Kind::Coerce) {
		info.kind = ArgInfo::Kind::ByVal;
		info.abiType = {convert(abc::PointerType::create(abcType))};
	    }
	}
	argInfo.push_back(std::move(info));
    }
    return argInfo;
}

static llvm::Type *
lowerReturnType(const abc::Type *retType)
{
    if (returnsInMemory(retType)) {
	return llvm::Type::getVoidTy(*llvmContext);
    }
    auto abiType = lowerType(retType);
    if (!abiType) {
	return convert(retType);
    }
    return abiType->size() == 1 ? abiType->front()
                                : llvm::StructType::get(*llvmContext, *abiType);
}

// lower function declaration
llvm::FunctionType *
lowerFunctionType(const abc::Type *abcFnType)
{
    std::vector<llvm::Type *> paramType;

    if (returnsInMemory(abcFnType->retType())) {
	paramType.push_back(llvm::PointerType::getUnqual(*llvmContext));
    }
    for (const auto &argInfo : classifyParamTypes(abcFnType)) {
	for (auto *ty : argInfo.abiType) {
	    paramType.push_back(ty);
	}
    }
    return llvm::FunctionType::get(lowerReturnType(abcFnType->retType()),
                                   paramType, abcFnType->hasVarg());
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
    if (returnsInMemory(abcFnType->retType())) {
	fn->addParamAttr(abiIndex++,
	                 llvm::Attribute::getWithStructRetType(
	                     *llvmContext, convert(abcFnType->retType())));
    }
    for (const auto &argInfo : classifyParamTypes(abcFnType)) {
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

// x86-64 ABI classification operates on eightbytes
static Value
eightbyteAddress(Value addr, std::size_t index)
{
    return llvmBuilder->CreateConstGEP1_64(llvm::Type::getInt8Ty(*llvmContext),
                                           addr, index * 8);
}

static llvm::Align
eightbyteAlign(llvm::Align align, std::size_t index)
{
    return llvm::commonAlignment(align, index * 8);
}

static void
lowerArgument(Value arg, const ArgInfo &argInfo, std::vector<Value> &abiArg)
{
    if (argInfo.kind == ArgInfo::Kind::Coerce) {
	for (std::size_t i = 0; i < argInfo.abiType.size(); ++i) {
	    abiArg.push_back(llvmBuilder->CreateAlignedLoad(
	        argInfo.abiType[i], eightbyteAddress(arg, i),
	        eightbyteAlign(argInfo.align, i)));
	}
    } else {
	abiArg.push_back(arg);
    }
}

static Value
createReturnTemporary(const abc::Type *retType)
{
    auto fn = llvmBuilder->GetInsertBlock()->getParent();
    llvm::IRBuilder<> entry(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    auto addr = entry.CreateAlloca(convert(retType));
    addr->setAlignment(getAlignof(retType));
    return addr;
}

// The caller expects a value of the aggregate type
static Value
reconstructReturnValue(Value call, const abc::Type *retType,
                       const std::vector<llvm::Type *> &abiType)
{
    auto addr = createReturnTemporary(retType);
    auto align = getAlignof(retType);

    for (std::size_t i = 0; i < abiType.size(); ++i) {
	auto val =
	    abiType.size() == 1 ? call : llvmBuilder->CreateExtractValue(call, i);
	llvmBuilder->CreateAlignedStore(val, eightbyteAddress(addr, i),
	                                eightbyteAlign(align, i));
    }
    return fetch(addr, retType);
}

void
returnInstruction(llvm::Function *fn, Value addr, const abc::Type *retType)
{
    if (returnsInMemory(retType)) {
	store(fetch(addr, retType), fn->getArg(0), retType);
	llvmBuilder->CreateRetVoid();
	return;
    }
    auto abiType = lowerType(retType);
    if (!abiType) {
	llvmBuilder->CreateRet(fetch(addr, retType));
	return;
    }
    auto align = getAlignof(retType);
    auto load = [&](std::size_t i) -> Value {
	return llvmBuilder->CreateAlignedLoad((*abiType)[i],
	                                      eightbyteAddress(addr, i),
	                                      eightbyteAlign(align, i));
    };
    if (abiType->size() == 1) {
	llvmBuilder->CreateRet(load(0));
	return;
    }
    Value val = llvm::PoisonValue::get(lowerReturnType(retType));
    for (std::size_t i = 0; i < abiType->size(); ++i) {
	val = llvmBuilder->CreateInsertValue(val, load(i), i);
    }
    llvmBuilder->CreateRet(val);
}

Value
lowerFunctionCall(Value fnAddr, const abc::Type *fnAbcType,
                  const std::vector<Value> &arg)
{
    assert(fnAbcType);
    auto llvmFnType = lowerFunctionType(fnAbcType);
    assert(llvmFnType);

    auto argInfo = classifyParamTypes(fnAbcType);

    auto retType = fnAbcType->retType();
    bool inMemory = returnsInMemory(retType);

    std::vector<Value> abiArg;
    Value retAddr = nullptr;
    if (inMemory) {
	retAddr = createReturnTemporary(retType);
	abiArg.push_back(retAddr);
    }
    for (std::size_t i = 0; i < argInfo.size(); ++i) {
	lowerArgument(arg[i], argInfo[i], abiArg);
    }
    for (std::size_t i = argInfo.size(); i < arg.size(); ++i) {
	abiArg.push_back(arg[i]);
    }

    auto fnCall = llvmBuilder->CreateCall(llvmFnType, fnAddr, abiArg);

    std::size_t abiIndex = 0;
    bool hasByVal = false;
    if (inMemory) {
	fnCall->addParamAttr(abiIndex++, llvm::Attribute::getWithStructRetType(
	                                     *llvmContext, convert(retType)));
    }
    for (const auto &info : argInfo) {
	if (info.kind == ArgInfo::Kind::ByVal) {
	    assert(info.abiType.size() == 1);
	    fnCall->addParamAttr(abiIndex,
	                         llvm::Attribute::getWithByValType(
	                             *llvmContext, convert(info.abcType)));
	    fnCall->addParamAttr(abiIndex, llvm::Attribute::getWithAlignment(
	                                       *llvmContext, info.align));
	    hasByVal = true;
	}
	abiIndex += info.abiType.size();
    }
    // Workaround for an LLVM miscompile (LLVM 17 to at least 24,
    // https://github.com/llvm/llvm-project/issues/235074, see
    // doc/llvm-issues/tailcallelim-byval): tail recursion elimination writes
    // to byval arguments but keeps memory(argmem: read), so the inliner elides
    // the byval copy and the caller's object gets modified.  notail keeps
    // tailcallelim away from calls with byval arguments.
    if (hasByVal) {
	fnCall->setTailCallKind(llvm::CallInst::TCK_NoTail);
    }
    if (inMemory) {
	return fetch(retAddr, retType);
    }
    if (auto abiType = lowerType(retType)) {
	return reconstructReturnValue(fnCall, retType, *abiType);
    }
    return fnCall;
}

static void
reconstructArgument(const llvm::Function *fnDecl, std::size_t abiIndex,
                    Value addr, const ArgInfo &argInfo)
{
    if (argInfo.kind == ArgInfo::Kind::ByVal) {
	// the argument points to the callee's private copy (byval): copy the
	// complete aggregate (argInfo.abiType[0] is just the pointer type)
	auto tmp = fetch(fnDecl->getArg(abiIndex), argInfo.abcType);

	store(tmp, addr, argInfo.abcType);
    } else if (argInfo.kind == ArgInfo::Kind::Coerce) {
	for (std::size_t j = 0; j < argInfo.abiType.size(); ++j) {
	    llvmBuilder->CreateAlignedStore(fnDecl->getArg(abiIndex + j),
	                                    eightbyteAddress(addr, j),
	                                    eightbyteAlign(argInfo.align, j));
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
    std::size_t abiIndex = returnsInMemory(abcFnType->retType()) ? 1 : 0;
    auto argInfo = classifyParamTypes(abcFnType);
    for (std::size_t i = 0; i < param.size(); ++i) {
	auto addr =
	    localVariableDefinition(param[i], abcFnType->paramType()[i]);
	reconstructArgument(fnDecl, abiIndex, addr, argInfo[i]);
	abiIndex += argInfo[i].abiType.size();
    }
}

} // namespace abi
} // namespace gen
