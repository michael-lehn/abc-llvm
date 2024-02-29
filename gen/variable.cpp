#include <iostream>
#include <unordered_map>

#include "convert.hpp"
#include "function.hpp"
#include "variable.hpp"

namespace gen {

// Map with all local variables
static std::unordered_map<const char *, llvm::AllocaInst *> localVariable;

//------------------------------------------------------------------------------

void
externalVariableiDeclaration(const char *ident, const abc::Type *varType)
{
    assert(llvmModule);
    assert(!varType->isFunction());

    auto linkage = llvm::GlobalValue::ExternalLinkage;

    if (auto found = loadAddress(ident)) {
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

    if (auto found = loadAddress(ident)) {
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

    auto llvmVarType = convert(varType);
    assert(llvmVarType);

    new llvm::GlobalVariable(*llvmModule,
			     llvmVarType,
			     /*isConstant=*/false,
			     /*Linkage=*/linkage,
			     /*Initializer=*/initialValue,
			     /*Name=*/ident,
			     nullptr);
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

Value
loadAddress(const char *ident)
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
