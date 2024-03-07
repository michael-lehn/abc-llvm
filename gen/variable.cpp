#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "type/integertype.hpp"

#include "constant.hpp"
#include "convert.hpp"
#include "function.hpp"
#include "variable.hpp"

namespace gen {

// Map with all string literals
static std::unordered_map<std::string, std::string> stringMap;

// Map with all local variables
static std::unordered_map<const char *, llvm::AllocaInst *> localVariable;
static Value lookup(const char *ident);

//------------------------------------------------------------------------------

void
externalVariableiDeclaration(const char *ident, const abc::Type *varType)
{
    assert(llvmModule);
    assert(!varType->isFunction());

    auto linkage = llvm::GlobalValue::ExternalLinkage;

    if (auto found = lookup(ident)) {
	// variable exists, assert it is a global variable
	auto var = llvm::dyn_cast<llvm::GlobalVariable>(found);
	assert(var);
	// extern declaration can be followed by static declaration
	// static declaration *can not* be followed by extern declaration
	assert(var->getLinkage() == llvm::Function::ExternalLinkage);
	return;
    }

    auto llvmVarType = convert(varType);
    assert(llvmVarType);

    new llvm::GlobalVariable(*llvmModule,
			     llvmVarType,
			     /*isConstant=*/false,
			     /*Linkage=*/linkage,
			     /*Initializer=*/nullptr,
			     /*Name=*/ident,
			     nullptr);
}

void
globalVariableDefinition(const char *ident, const abc::Type *varType,
			 Constant initialValue, bool externalLinkage)
{
    assert(llvmModule);
    assert(!varType->isFunction());

    auto linkage = externalLinkage
	? llvm::GlobalValue::ExternalLinkage
	: llvm::GlobalValue::InternalLinkage;

    auto llvmVarType = convert(varType);
    assert(llvmVarType);

    if (!initialValue) {
	initialValue = llvm::Constant::getNullValue(llvmVarType);
    }

    if (auto found = lookup(ident)) {
	// variable exists, assert it is a global variable
	auto var = llvm::dyn_cast<llvm::GlobalVariable>(found);
	assert(var);
	// extern declaration can be followed by static declaration
	// static declaration *can not* be followed by extern declaration
	assert(!externalLinkage
		    || var->getLinkage() == llvm::Function::ExternalLinkage);
	var->setInitializer(initialValue);
	return;
    }

    new llvm::GlobalVariable(*llvmModule,
			     llvmVarType,
			     /*isConstant=*/false,
			     /*Linkage=*/linkage,
			     /*Initializer=*/initialValue,
			     /*Name=*/ident,
			     nullptr);
}

Value
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
    return loadAddress(stringMap.at(str).c_str());
}

Value
localVariableDefinition(const char *ident, const abc::Type *varType)
{
    assert(varType);
    assert(!varType->isFunction());
    assert(functionBuildingInfo.fn);

    // always allocate memory at entry of function
    auto fn = functionBuildingInfo.fn;
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(),
				 fn->getEntryBlock().begin());
    auto llvmVarType = convert(varType);
    localVariable[ident] = tmpBuilder.CreateAlloca(llvmVarType, nullptr, ident);
    return localVariable[ident];
}

void
forgetAllLocalVariables()
{
    localVariable.clear();
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

Value
pointerIncrement(const abc::Type *type, Value pointer, Value offset)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    auto llvmType = convert(type);

    std::vector<Value> idxList{1};
    idxList[0] = offset;

    return llvmBuilder->CreateGEP(llvmType, pointer, idxList);
}

Value
pointerDifference(const abc::Type *type, Value pointer1, Value pointer2)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
    auto llvmType = convert(type);

    return llvmBuilder->CreatePtrDiff(llvmType, pointer1, pointer2);
}

Value
pointerToIndex(const abc::Type *type, Value pointer, std::size_t index)
{
    assert(llvmBuilder);
    assert(!functionBuildingInfo.bbClosed);
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
