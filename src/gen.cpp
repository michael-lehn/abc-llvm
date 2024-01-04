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
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"

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

static auto llvmFPM = std::make_unique<llvm::FunctionPassManager>();
static auto llvmLAM = std::make_unique<llvm::LoopAnalysisManager>();
static auto llvmFAM = std::make_unique<llvm::FunctionAnalysisManager>();
static auto llvmCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
static auto llvmMAM = std::make_unique<llvm::ModuleAnalysisManager>();
static auto llvmPIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
static auto llvmSI = std::make_unique<llvm::StandardInstrumentations>(
			*llvmContext, /*DebugLogging*/ true);

static struct InitPM {
    InitPM()
    {
	llvmSI->registerCallbacks(*llvmPIC, llvmMAM.get());
	llvmFPM->addPass(llvm::InstCombinePass());
	llvmFPM->addPass(llvm::ReassociatePass());
	llvmFPM->addPass(llvm::GVNPass());
	llvmFPM->addPass(llvm::SimplifyCFGPass());

	llvm::PassBuilder pb;
	pb.registerModuleAnalyses(*llvmMAM);
	pb.registerFunctionAnalyses(*llvmFAM);
	pb.crossRegisterProxies(*llvmLAM, *llvmFAM, *llvmCGAM, *llvmMAM);
    }
} initPm;

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

static struct {
    llvm::Function *llvmFn;
    Label leave;
    const Type *retType;
} currFn;

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

    // set current function
    currFn.llvmFn = fn;
    currFn.leave = getLabel("leave");
    if ((currFn.retType = fnType->getRetType())) {
	// reserve space for return value on stack
	allocLocal(".retVal", currFn.retType);
    }

}

void
fnDefEnd(void)
{
    assert(currFn.llvmFn);

    // leave function
    jmp(currFn.leave);
    labelDef(currFn.leave);

    if (currFn.retType) {
	auto ret = fetch(".retVal", currFn.retType);
	llvmBuilder->CreateRet(ret);
    } else {
	llvmBuilder->CreateRetVoid();
    }

    // optimize function code
    llvm::verifyFunction(*currFn.llvmFn);
    //llvmFPM->run(*currFn.llvmFn, *llvmFAM);
    //llvmFPM->run(*currFn.llvmFn, *llvmFAM);
}

void
ret(Reg reg)
{
    if (reg) {
	store(reg, ".retVal", currFn.retType);
    }
    jmp(currFn.leave);
}


//------------------------------------------------------------------------------

Cond
cond(CondOp op, Reg a, Reg b)
{
    switch (op) {
	case EQ:
	    return llvmBuilder->CreateICmpEQ(a, b);
	case NE:
	    return llvmBuilder->CreateICmpNE(a, b);
	case SGT:
	    return llvmBuilder->CreateICmpSGT(a, b);
    	case SGE:
	    return llvmBuilder->CreateICmpSGE(a, b);
    	case SLT:
	    return llvmBuilder->CreateICmpSLT(a, b);
    	case SLE:
	    return llvmBuilder->CreateICmpSLE(a, b);
    	case UGT:
	    return llvmBuilder->CreateICmpUGT(a, b);
    	case UGE:
	    return llvmBuilder->CreateICmpUGE(a, b);
    	case ULT:
	    return llvmBuilder->CreateICmpULT(a, b);
    	case ULE:
	    return llvmBuilder->CreateICmpULE(a, b);
    }
    assert(0);
    return nullptr; // never reached
}

Label
getLabel(const char *name)
{
    return llvm::BasicBlock::Create(*llvmContext, name);
}

void
labelDef(Label label)
{
    currFn.llvmFn->insert(currFn.llvmFn->end(), label);
    llvmBuilder->SetInsertPoint(label);
}

void
jmp(Label label)
{
    llvmBuilder->CreateBr(label);
}

void
jmp(Cond cond, Label trueLabel, Label falseLabel)
{
    llvmBuilder->CreateCondBr(cond, trueLabel, falseLabel);
}

Reg
phi(Reg a, Label labelA, Reg b, Label labelB, const Type *type)
{
    auto ty = TypeMap::get(type);
    auto phi = llvmBuilder->CreatePHI(ty, 2);
    phi->addIncoming(a, labelA);
    phi->addIncoming(b, labelB);
    return phi;
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

Reg
fetch(const char *ident, const Type *type)
{
    auto ty = TypeMap::get(type);
    auto var = local[ident];
    return llvmBuilder->CreateLoad(ty, var, ident);
}

void
store(Reg reg, const char *ident, const Type *type)
{
    auto var = local[ident];
    llvmBuilder->CreateStore(reg, var);
}

//------------------------------------------------------------------------------

Reg
loadConst(const char *val, const Type *type)
{
    if (type->isInteger()) {
	auto apint = llvm::APInt(type->getIntegerNumBits(), val, 10);
	return llvm::ConstantInt::get(*llvmContext, apint);
    }
    return nullptr;
}

//------------------------------------------------------------------------------

Reg
aluInstr(AluOp op, Reg l, Reg r)
{
    switch (op) {
	case ADD:
	    return llvmBuilder->CreateAdd(l, r);
	case SUB:
	    return llvmBuilder->CreateSub(l, r);
	case SMUL:
	    return llvmBuilder->CreateMul(l, r);
	case SDIV:
	    return llvmBuilder->CreateSDiv(l, r);
	case SMOD:
	    return llvmBuilder->CreateSRem(l, r);
	case UDIV:
	    return llvmBuilder->CreateUDiv(l, r);
	case UMOD:
	    return llvmBuilder->CreateURem(l, r);
	default:
	    assert(0);
	    return nullptr;
    }
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
	    targetTriple,
	    "generic",
	    "",
	    llvm::TargetOptions{},
	    llvm::Reloc::PIC_,
	    std::nullopt,
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
    llvmModule->print(llvm::errs(), nullptr);
}


} // namespace gen
