#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/Analysis/ConstantFolding.h"

#include "type/integertype.hpp"

#include "constant.hpp"
#include "function.hpp"
#include "gentype.hpp"
#include "variable.hpp"

namespace gen {

// Map with all string literals
static std::unordered_map<std::string, std::string> stringMap;

// Map with all local variables
static std::unordered_map<const char *, llvm::AllocaInst *> localVariable;
static std::unordered_map<llvm::AllocaInst *, const abc::Type *> allocaAbcType;

static Value lookup(const char *ident);

//------------------------------------------------------------------------------

bool
externalVariableDeclaration(const char *ident, const abc::Type *varType)
{
    assert(llvmModule);
    assert(!varType->isFunction());

    if (auto found = lookup(ident)) {
	// variable exists, assert it is a global variable
	auto var = llvm::dyn_cast<llvm::GlobalVariable>(found);
	assert(var);
	// extern declaration can be followed by static declaration
	// static declaration *can not* be followed by extern declaration
	return var->getLinkage() == llvm::Function::ExternalLinkage;
    }

    auto llvmVarType = convert(varType);
    assert(llvmVarType);

    new llvm::GlobalVariable(*llvmModule,
			     llvmVarType,
			     /*isConstant=*/false,
			     /*Linkage=*/llvm::GlobalValue::ExternalLinkage,
			     /*Initializer=*/nullptr,
			     /*Name=*/ident,
			     nullptr);
    return true;
}

void
globalVariableDefinition(const char *ident, const abc::Type *varType,
			 Constant initialValue)
{
    assert(llvmModule);
    assert(!varType->isFunction());

    auto llvmVarType = convert(varType);
    assert(llvmVarType);

    if (!initialValue) {
	initialValue = llvm::Constant::getNullValue(llvmVarType);
    }

    if (auto found = lookup(ident)) {
	// variable exists, assert it is a global variable
	auto var = llvm::dyn_cast<llvm::GlobalVariable>(found);
	assert(var);
	var->setInitializer(initialValue);
	return;
    }

    new llvm::GlobalVariable(*llvmModule,
			     llvmVarType,
			     /*isConstant=*/false,
			     /*Linkage=*/llvm::GlobalValue::InternalLinkage,
			     /*Initializer=*/initialValue,
			     /*Name=*/ident,
			     nullptr);
}

Constant
loadStringAddress(const char *stringLiteral)
{
    assert(llvmContext);
    assert(llvmModule);

    std::string str{stringLiteral};
    if (!stringMap.contains(str)) {
	std::stringstream ss;
	ss << ".L" << stringMap.size();
	stringMap[str] = ss.str();
	auto llvmStr = llvm::ConstantDataArray::getString(*llvmContext,
							  stringLiteral);
	new llvm::GlobalVariable(*llvmModule,
				 llvmStr->getType(),
				 /*isConstant=*/true,
				 /*Linkage=*/llvm::GlobalValue::InternalLinkage,
				 /*Initializer=*/llvmStr,
				 /*Name=*/ss.str().c_str());
    }
    return loadConstantAddress(stringMap.at(str).c_str());
}

Value
localVariableDefinition(const char *ident, const abc::Type *varType)
{
    assert(varType);
    assert(!varType->isFunction());
    assert(functionBuildingInfo.fn);

    auto llvmVarType = convert(varType);
    if (localVariable.contains(ident)) {
	auto val = localVariable.at(ident);
	assert(val->getAllocatedType() == llvmVarType);
	return val;
    }

    // always allocate memory at entry of function
    auto fn = functionBuildingInfo.fn;
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(),
				 fn->getEntryBlock().begin());
    localVariable[ident] = tmpBuilder.CreateAlloca(llvmVarType, nullptr, ident);
    allocaAbcType[localVariable[ident]] = varType;
    return localVariable[ident];
}

static void
insertDestructorAfterLastUse(llvm::AllocaInst *alloca, const abc::Type *type)
{
    if (alloca->getName().str() == ".retVal") {
	return;
    }
    llvm::Instruction *lastUse = nullptr;

    for (auto *User : alloca->users()) {
	if (auto instr = dyn_cast<llvm::Instruction>(User)) {
	    if (!lastUse || lastUse->comesBefore(instr)) {
		lastUse = instr;
	    }
	}
    }

    if (lastUse) {
	lastUse = lastUse->getNextNode();
	assert(lastUse);
	llvm::IRBuilder<> builder(lastUse);
	
	auto destrName = type->destructorName().c_str();
	auto destr = llvmModule->getFunction(destrName);
	if (destr) {
	    builder.CreateCall(destr, {alloca});
	} else {
	    std::cerr << "function '" << destrName << "' not found\n";
	    assert(0);
	}
    }
}
void
releaseLocalVariables()
{
    for (auto &[alloca, type]: allocaAbcType) {
	if (type->isUPointer()) {
	    insertDestructorAfterLastUse(alloca, type);
	}
    }
    forgetAllLocalVariables();
}

void
forgetAllVariables()
{
    stringMap.clear();
    forgetAllLocalVariables();
}

void
forgetAllLocalVariables()
{
    localVariable.clear();
    allocaAbcType.clear();
}

static Value
lookup(const char *ident)
{
    assert(llvmModule);
    if (auto var = llvmModule->getGlobalVariable(ident, true)) {
	return var;
    } else if (auto fn = llvmModule->getFunction(ident)) {
	return fn;
    } else if (localVariable.contains(ident)) {
	return localVariable.at(ident);
    } else {
	return nullptr;
    }
}

bool
hasConstantAddress(const char *ident)
{
    return loadConstantAddress(ident);
}

Constant
loadConstantAddress(const char *ident)
{
    assert(llvmModule);
    if (auto var = llvmModule->getGlobalVariable(ident, true)) {
	return var;
    } else if (auto fn = llvmModule->getFunction(ident)) {
	return fn;
    } else {
	return nullptr;
    }
}

Value
loadAddress(const char *ident)
{
    auto addr = lookup(ident);
    if (!addr) {
	std::cerr << "gen::loadAddress: identifier "
	    << ident << " was not defined.\n";
	assert(0);
    }
    return addr;
}

Constant
pointerIncrement(const abc::Type *type, Constant pointer, std::uint64_t offset)
{
    assert(llvmBuilder);
    assert(type);
    auto llvmType = convert(type);

    auto addr =  llvmBuilder->CreateConstGEP1_64(llvmType, pointer, offset);
    return llvm::dyn_cast<llvm::Constant>(addr);
}

Value
pointerIncrement(const abc::Type *type, Value pointer, Value offset)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    assert(type);
    auto llvmType = convert(type);

    std::vector<Value> idxList{1};
    idxList[0] = offset;

    return llvmBuilder->CreateGEP(llvmType, pointer, idxList);
}

std::optional<Constant>
pointerConstantDifference(const abc::Type *type, Value pointer1, Value pointer2)
{
    assert(llvmModule);
    auto llvmType = convert(type);
    auto dl = llvmModule->getDataLayout();
    if (auto diff = pointer1->getPointerOffsetFrom(pointer2, dl)) {
	return getConstantInt(diff.value() / dl.getTypeAllocSize(llvmType),
			      abc::IntegerType::createPtrdiffType());
    } else {
	return std::nullopt;
    }
}

Value
pointerDifference(const abc::Type *type, Value pointer1, Value pointer2)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    assert(type);
    auto llvmType = convert(type);
    return llvmBuilder->CreatePtrDiff(llvmType, pointer1, pointer2);
}

Value
pointerToIndex(const abc::Type *type, Value pointer, std::size_t index)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    assert(type);
    assert(type->isStruct());
    auto llvmType = convert(type);

    std::vector<Value> idxList{2};
    idxList[0] = getConstantZero(abc::IntegerType::createSigned(8));
    idxList[1] = getConstantInt(index, abc::IntegerType::createUnsigned(32));

    return llvmBuilder->CreateGEP(llvmType, pointer, idxList);
}

Value
fetch(Value addr, const abc::Type *type)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    assert(type);
    auto llvmType = convert(type);
    return llvmBuilder->CreateLoad(llvmType, addr);
}

Value
store(Value val, Value addr)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    llvmBuilder->CreateStore(val, addr);
    return val;
}

void
printGlobalVariableList()
{
    std::cerr << "printGlobalVariableList:\n";
    for (const auto &var : llvmModule->globals()) {
	std::cerr << "ident: " << var.getGlobalIdentifier() << std::endl;
    }
    std::cerr << "----\n\n";
}

void
printFunctionList()
{
    std::cerr << "printFunctionList:\n";
    for (const auto &fn : llvmModule->functions()) {
	std::cerr << "ident: " << fn.getName().str() << std::endl;
    }
    std::cerr << "----\n\n";
}

} // namespace gen
