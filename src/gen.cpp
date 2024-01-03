#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <system_error>
#include <unordered_map>

#include "gen.hpp"

namespace gen {

static auto llvmContext = std::make_unique<llvm::LLVMContext>();
static auto llvmModule = std::make_unique<llvm::Module>("llvm", *llvmContext);
static auto llvmBuilder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);
static llvm::BasicBlock *llvmBB;

struct TypeMap
{
    static std::unordered_map<const Type *, llvm::Type *> typeMap;

    static std::vector<llvm::Type *>
    get_(const std::vector<const Type *> &argType)
    {
	std::vector<llvm::Type *> llvmArgType{argType.size()};

	for (std::size_t i = 0; i < argType.size(); ++i) {
	    llvmArgType[i] = get_(argType[i]);
	}
	return llvmArgType;
    }

    static llvm::Type *
    get_(const Type *t)
    {
	if (!t) {
	    return llvm::Type::getVoidTy(*llvmContext);
	} else if (t->isInteger()) {
	    switch (t->getIntegerNumBits()) {
		case 8:
		    return llvm::Type::getInt8Ty(*llvmContext);
		case 16:
		    return llvm::Type::getInt16Ty(*llvmContext);
		case 32:
		    return llvm::Type::getInt32Ty(*llvmContext);
		case 64:
		    return llvm::Type::getInt64Ty(*llvmContext);
		default:
		    return llvm::Type::getIntNTy(*llvmContext,
			    t->getIntegerNumBits());
	    }
	} else if (t->isPointer()) {
	    return get_(t->getRefType())->getPointerTo();
	} else if (t->isFunction()) {
	    return llvm::FunctionType::get(get_(t->getRetType()),
		    get_(t->getArgType()), false);
	}
	assert(0);
	return 0;
    }

    static llvm::Type *
    get(const Type *t)
    {
	return typeMap.contains(t) ? typeMap[t] : typeMap[t] = get_(t);
    }

    template <typename T>
    static T *
    get(const Type *t)
    {
	return llvm::dyn_cast<T>(get(t));
    }

};

std::unordered_map<const Type *, llvm::Type *> TypeMap::typeMap;

//------------------------------------------------------------------------------

static llvm::Function *
makeFnDecl(const char *ident, const Type *fnType)
{
    if (auto fn = llvmModule->getFunction(ident)) {
	return fn;
    }

    auto linkage = llvm::Function::ExternalLinkage;
    auto llvmType = TypeMap::get<llvm::FunctionType>(fnType);
    return llvm::Function::Create(llvmType, linkage, ident, llvmModule.get());
}

void
fnDecl(const char *ident, const Type *fnType)
{
    makeFnDecl(ident, fnType);
}

void
fnDef(const char *ident, const Type *fnType,
      const std::vector<const char *> &param)
{
    auto fn = makeFnDecl(ident, fnType);
    fn->setDoesNotThrow();
    llvmBB = llvm::BasicBlock::Create(*llvmContext, "entry", fn);
    llvmBuilder->SetInsertPoint(llvmBB);

    // store param registers on stack
    auto argType = fnType->getArgType();
    assert(argType.size() == param.size());

    for (std::size_t i = 0; i < param.size(); ++i) {
	allocLocal(param[i], argType[i]);
	store(fn->getArg(i), param[i], argType[i]);
    }
}

//------------------------------------------------------------------------------
static std::unordered_map<std::string, llvm::AllocaInst *> local;

void
allocLocal(const char *ident, const Type *type)
{
    auto ty = TypeMap::get(type);
    local[ident] = llvmBuilder->CreateAlloca(ty, nullptr, ident);
}

//------------------------------------------------------------------------------

Reg *
fetch(const char *ident, const Type *type)
{
    auto ty = TypeMap::get(type);
    auto var = local[ident];
    return llvmBuilder->CreateLoad(ty, var, ident);
}

void
store(Reg *reg, const char *ident, const Type *type)
{
    auto var = local[ident];
    llvmBuilder->CreateStore(reg, var);
}

//------------------------------------------------------------------------------

Reg *
loadConst(const char *val, const Type *type)
{
    if (type->isInteger()) {
	auto apint = llvm::APInt(type->getIntegerNumBits(), val, 10);
	return llvm::ConstantInt::get(*llvmContext, apint);
    }
    return nullptr;
}

//------------------------------------------------------------------------------

Reg *
op2r(Op op, Reg *l, Reg *r)
{
    switch (op) {
	case Add:
	    return llvmBuilder->CreateAdd(l, r);
	case Sub:
	    return llvmBuilder->CreateSub(l, r);
	case SMul:
	    return llvmBuilder->CreateMul(l, r);
	case SDiv:
	    return llvmBuilder->CreateSDiv(l, r);
	default:
	    assert(0);
	    return nullptr;
    }
}

//------------------------------------------------------------------------------

void
ret(Reg *reg)
{
    assert(llvmBB);
    reg ? llvmBuilder->CreateRet(reg) : llvmBuilder->CreateRetVoid();
}

//------------------------------------------------------------------------------

void
dump_bc(const char *filename)
{
    std::error_code ec;
    auto f = llvm::raw_fd_ostream (std::string{filename} + ".bc", ec);

    if (ec) {
	llvm::errs() << "Could not open file: " << ec.message();
	std::exit(1);
    }

    llvmModule->print(f, nullptr);
}

void
dump_asm(const char *filename, int codeGenOptLevel)
{
    assert(codeGenOptLevel >= 0 && codeGenOptLevel <= 3);

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
	llvm::errs() << error;
	std::exit(1);
    }

    auto targetMachine = target->createTargetMachine(
	    targetTriple, "generic", "", llvm::TargetOptions{},
	    llvm::Reloc::PIC_, std::nullopt,
	    llvm::CodeGenOpt::Level{codeGenOptLevel});
    llvmModule->setDataLayout(targetMachine->createDataLayout());

    std::error_code ec;
    auto dest = llvm::raw_fd_ostream{
		    std::string{filename} + ".s",
		    ec,
		    llvm::sys::fs::OF_None
    };

    if (ec) {
	llvm::errs() << "Could not open file: " << ec.message();
	std::exit(1);
    }

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::CGFT_AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
	llvm::errs() << "TheTargetMachine can't emit a file of this type";
	std::exit(1);
    }
    pass.run(*llvmModule);
    dest.flush();
}


} // namespace gen
