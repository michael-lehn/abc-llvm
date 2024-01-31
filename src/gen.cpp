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
#include <cstring>
#include <iostream>
#include <system_error>
#include <unordered_map>

#include "error.hpp"
#include "gen.hpp"
#include "symtab.hpp"

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
    get_(const Type *t, std::size_t dim)
    {
	return llvm::ArrayType::get(get_(t), dim);
    }

    static llvm::Type *
    get_(const Type *t)
    {
	assert(t);
	if (t->isVoid()) {
	    return llvm::Type::getVoidTy(*llvmContext);
	} else if (t->isInteger()) {
	    switch (t->getIntegerNumBits()) {
		case 1:
		    return llvm::Type::getInt1Ty(*llvmContext);
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
	    if (t->isNullPointer()) {
		auto ty = llvm::Type::getInt64Ty(*llvmContext);
		return ty->getPointerTo();
	    }
	    return get_(t->getRefType())->getPointerTo();
	} else if (t->isArray()) {
	    return get_(t->getRefType(), t->getDim());
	} else if (t->isFunction()) {
	    return llvm::FunctionType::get(get_(t->getRetType()),
		    get_(t->getArgType()), t->hasVarg());
	} else if (t->isStruct()) {
	    auto type = t->getMemberType();
	    std::vector<llvm::Type *> llvmType(type.size());
	    for (std::size_t i = 0; i < type.size(); ++i) {
		llvmType[i] = type[i]->isPointer()
		    ? get(Type::getPointer(Type::getVoid()))
		    : get(type[i]);
	    }
	    return llvm::StructType::get(*llvmContext, llvmType);
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

static bool setTargetInit = false;
static bool opt;
static llvm::TargetMachine *targetMachine;

void
setOpt(bool enableOpt)
{
    opt = enableOpt;
}

void
setTarget(int codeGenOptLevel)
{
    assert(codeGenOptLevel >= 0 && codeGenOptLevel <= 3);
    setOpt(codeGenOptLevel);

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

    targetMachine = target->createTargetMachine(
	    targetTriple,
	    "generic",
	    "",
	    llvm::TargetOptions{},
	    llvm::Reloc::PIC_,
	    std::nullopt,
	    llvm::CodeGenOpt::Level{codeGenOptLevel});
    llvmModule->setDataLayout(targetMachine->createDataLayout());
    setTargetInit = true;
}


//------------------------------------------------------------------------------

// local variables
static std::unordered_map<std::string, llvm::AllocaInst *> local;

// global variables
static std::unordered_map<std::string, llvm::GlobalValue *> global;

static llvm::Function *
makeFnDecl(const char *ident, const Type *fnType)
{
    if (auto fn = llvmModule->getFunction(ident)) {
	return fn;
    }

    auto s = Symtab::get(ident, Symtab::RootScope);
    if (!s) {
	assert(0 && "symbol for function not declared in root scope");
    }

    auto linkage = s->hasExternFlag() || !strcmp(ident, "main")
	? llvm::Function::ExternalLinkage
	: llvm::Function::InternalLinkage;
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
    bool bbClosed;
} currFn;

//------------------------------------------------------------------------------

bool
openBuildingBlock()
{
    return !currFn.bbClosed;
}

static void
assureOpenBuildingBlock()
{
    if (!openBuildingBlock()) {
	labelDef(getLabel("notReached"));
    }
}

//------------------------------------------------------------------------------

void
fnDef(const char *ident, const Type *fnType,
      const std::vector<const char *> &param)
{
    if (auto s = Symtab::get(ident, Symtab::RootScope)) {
	s->setDefinitionFlag();
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for function not declared in root scope");
    }

    local.clear();

    auto fn = makeFnDecl(ident, fnType);
    fn->setDoesNotThrow();
    llvmBB = llvm::BasicBlock::Create(*llvmContext, "entry", fn);
    llvmBuilder->SetInsertPoint(llvmBB);

    // set current function
    currFn.llvmFn = fn;
    currFn.leave = getLabel("leave");
    currFn.retType = fnType->getRetType();
    if (!currFn.retType->isVoid()) {
	// reserve space for return value on stack
	defLocal(".retVal", currFn.retType);
    }

    // store param registers on stack
    auto argType = fnType->getArgType();
    assert(argType.size() == param.size());

    for (std::size_t i = 0; i < param.size(); ++i) {
	defLocal(param[i], argType[i]);
	store(fn->getArg(i), param[i], argType[i]);
    }

    // add function to global variables
    global[ident] = fn;
}

void
fnDefEnd(void)
{
    assert(currFn.llvmFn);

    // leave function
    ret(nullptr);
    labelDef(currFn.leave);

    if (currFn.retType->isVoid()) {
	llvmBuilder->CreateRetVoid();
    } else {
	auto ret = fetch(".retVal", currFn.retType);
	llvmBuilder->CreateRet(ret);
    }

    // optimize function code
    llvm::verifyFunction(*currFn.llvmFn);
    if (opt) {
	llvmFPM->run(*currFn.llvmFn, *llvmFAM);
	llvmFPM->run(*currFn.llvmFn, *llvmFAM);
    }
}

void
ret(Reg reg)
{
    assert(currFn.llvmFn);

    if (reg) {
	store(reg, ".retVal", currFn.retType);
    }
    jmp(currFn.leave);
}

void
defLocal(const char *ident, const Type *type)
{
    assert(currFn.llvmFn);
    assureOpenBuildingBlock();

    if (auto s = Symtab::get(ident, Symtab::CurrentScope)) {
	s->setDefinitionFlag();
	ident = s->getInternalIdent().c_str();
    } else {
	error::out() << "internal error: Symbol '" << ident
	    << "' not found in current scope" << std::endl;
	Symtab::print(error::out());
	assert(0 && "symbol for local variable not declared in current scope");
    }

    if (type->isFunction()) {
	error::out() << "Function can not be defined as local variable"
	    << std::endl;
	error::fatal();
    }

    // always allocate memory at entry of function
    llvm::IRBuilder<> tmpBuilder(&currFn.llvmFn->getEntryBlock(),
				 currFn.llvmFn->getEntryBlock().begin());
    auto ty = TypeMap::get(type);
    local[ident] = tmpBuilder.CreateAlloca(ty, nullptr, ident);
}

void
declGlobal(const char *ident, const Type *type)
{
    if (type->isFunction()) {
	fnDecl(ident, type);
	return;
    }

    auto s = Symtab::get(ident, Symtab::RootScope);
    if (s) {
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for global variable not declared in root scope");
    }

    auto linkage = s->hasExternFlag()
	? llvm::GlobalValue::ExternalLinkage
	: llvm::GlobalValue::InternalLinkage;

    auto ty = TypeMap::get(type);
    global[ident] = new llvm::GlobalVariable(
			*llvmModule,
			 ty,
			/*isConstant=*/false,
			/*Linkage=*/linkage,
			/*Initializer=*/nullptr,
			/*Name=*/ident,
			nullptr);
}

void
defGlobal(const char *ident, const Type *type, ConstVal val)
{
    if (type->isFunction()) {
	fnDecl(ident, type);
	return;
    }

    auto s = Symtab::get(ident);
    if (s) {
	s->setDefinitionFlag();
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for global variable not declared in root scope");
    }

    auto linkage = s->hasExternFlag()
	? llvm::GlobalValue::ExternalLinkage
	: llvm::GlobalValue::InternalLinkage;

    auto ty = TypeMap::get(type);
    if (!val) {
	val = llvm::ConstantInt::getNullValue(ty);
    }
    if (global.contains(ident)) {
	auto v = llvm::dyn_cast<llvm::GlobalVariable>(global[ident]);
	v->setInitializer(val);
	return;
    }

    global[ident] = new llvm::GlobalVariable(
			*llvmModule,
			 ty,
			/*isConstant=*/false,
			/*Linkage=*/linkage,
			/*Initializer=*/val,
			/*Name=*/ident,
			nullptr);
}

void
defStringLiteral(const char *ident, const char *val, bool isConst)
{
    auto constVal = llvm::ConstantDataArray::getString(*llvmContext, val);
    global[ident] = new llvm::GlobalVariable(*llvmModule,
			constVal->getType(),
			/*isConstant=*/isConst,
			/*Linkage=*/llvm::GlobalValue::InternalLinkage,
			/*Initializer=*/constVal,
			/*Name=*/ident);
}

//------------------------------------------------------------------------------

// size of type
std::size_t
getSizeOf(const Type *type)
{
    auto ty = TypeMap::get(type);
    return llvmModule->getDataLayout().getTypeAllocSize(ty);
}

//------------------------------------------------------------------------------


Reg
call(const char *ident, const std::vector<Reg> &param)
{
    assureOpenBuildingBlock();
    if (auto s = Symtab::get(ident, Symtab::RootScope)) {
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for function not declared in root scope");
    }

    auto fn = llvmModule->getFunction(ident);
    assert(fn && "function not declared");

    return llvmBuilder->CreateCall(fn, param);
}

Reg
call(Reg fnPtr, const Type *fnType, const std::vector<Reg> &param)
{
    assert(fnType->isFunction());
    auto fnTy =  llvm::dyn_cast<llvm::FunctionType>(TypeMap::get(fnType));
    return llvmBuilder->CreateCall(fnTy, fnPtr, param);

}

//------------------------------------------------------------------------------

Cond
cond(CondOp op, Reg a, Reg b)
{
    assureOpenBuildingBlock();
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
    	case AND:
	    return llvmBuilder->CreateAnd(a, b);
    	case OR:
	    return llvmBuilder->CreateOr(a, b);
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
    if (!currFn.bbClosed) {
	jmp(label);
    }

    currFn.llvmFn->insert(currFn.llvmFn->end(), label);
    llvmBuilder->SetInsertPoint(label);
    currFn.bbClosed = false;
}

Label
jmp(Label &label)
{
    assureOpenBuildingBlock();
    auto ib = llvmBuilder->GetInsertBlock();
    if (!currFn.bbClosed) {
	llvmBuilder->CreateBr(label);
    }
    currFn.bbClosed = true;
    return ib;
}

void
jmp(Cond cond, Label trueLabel, Label falseLabel)
{
    assureOpenBuildingBlock();
    if (!currFn.bbClosed) {
	llvmBuilder->CreateCondBr(cond, trueLabel, falseLabel);
    }
    currFn.bbClosed = true;
}

void
jmp(Cond cond, Label defaultLabel,
    const std::vector<std::pair<ConstIntVal, Label>> &caseLabel)
{
    assureOpenBuildingBlock();
    auto sw = llvmBuilder->CreateSwitch(cond, defaultLabel, caseLabel.size());
    for (const auto &[val, label]: caseLabel) {
	sw->addCase(val, label);
    }
    currFn.bbClosed = true;
}

Reg
phi(Reg a, Label labelA, Reg b, Label labelB, const Type *type)
{
    assureOpenBuildingBlock();

    auto ty = TypeMap::get(type);
    auto phi = llvmBuilder->CreatePHI(ty, 2);
    phi->addIncoming(a, labelA);
    phi->addIncoming(b, labelB);
    return phi;
}

//------------------------------------------------------------------------------

Reg
loadAddr(const char *ident)
{
    assureOpenBuildingBlock();
    if (auto s = Symtab::get(ident)) {
	ident = s->getInternalIdent().c_str();
    } else {
	error::out() << "internal error: symbol '" << ident
	    << "' not found in symbol table" << std::endl;
	Symtab::print(error::out());
	assert(0 && "symbol for not declared in any scope");
    }

    auto addr = local.contains(ident)
	? llvm::dyn_cast<llvm::Value>(local[ident])
	: llvm::dyn_cast<llvm::Value>(global[ident]);
    assert(addr);
    return addr;
}

Reg
fetch(const char *ident, const Type *type)
{
    assureOpenBuildingBlock();
    if (auto s = Symtab::get(ident)) {
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for not declared in any scope");
    }

    auto ty = TypeMap::get(type);
    auto addr = local.contains(ident)
	? llvm::dyn_cast<llvm::Value>(local[ident])
	: llvm::dyn_cast<llvm::Value>(global[ident]);
    return llvmBuilder->CreateLoad(ty, addr, ident);
}

Reg
fetch(Reg addr, const Type *type)
{
    assureOpenBuildingBlock();
    auto ty = TypeMap::get(type);
    return llvmBuilder->CreateLoad(ty, addr);
}

void
store(Reg val, const char *ident, const Type *type)
{
    assureOpenBuildingBlock();
    if (auto s = Symtab::get(ident)) {
	ident = s->getInternalIdent().c_str();
    } else {
	assert(0 && "symbol for not declared in any scope");
    }

    auto addr = local.contains(ident)
	? llvm::dyn_cast<llvm::Value>(local[ident])
	: llvm::dyn_cast<llvm::Value>(global[ident]);
    llvmBuilder->CreateStore(val, addr);
}

void
store(Reg val, Reg addr, const Type *type)
{
    assureOpenBuildingBlock();
    llvmBuilder->CreateStore(val, addr);
}

//------------------------------------------------------------------------------

Reg
cast(Reg reg, const Type *fromType, const Type *toType)
{
    assureOpenBuildingBlock();

    fromType = Type::getConstRemoved(fromType);
    toType = Type::getConstRemoved(toType);

    if (fromType == toType) {
	return reg;
    }

    auto ty = TypeMap::get(toType);
    if (fromType->isInteger() && toType->isInteger()) {
	auto toNumBits = toType->getIntegerNumBits();
	auto fromNumBits = fromType->getIntegerNumBits();

	if (toNumBits > fromNumBits) {
	    return fromType->getIntegerKind() == Type::UNSIGNED
		? llvmBuilder->CreateZExtOrBitCast(reg, ty)
		: llvmBuilder->CreateSExtOrBitCast(reg, ty); 
	}
	return llvmBuilder->CreateTruncOrBitCast(reg, ty); 
    } else if (fromType->isFunction() && toType->isPointer()) {
	return reg;
    } else if (fromType->isArray() && toType->isPointer()) {
	return reg;
    } else if (fromType->isPointer() && toType->isArray()) {
	return reg;
    } else if (fromType->isPointer() && toType->isPointer()) {
	return reg;
    } else if (Type::convertArrayOrFunctionToPointer(fromType)->isPointer()
	    && toType->isInteger()) {
	return llvmBuilder->CreatePointerCast(reg, ty);
    } else if (fromType->isInteger()
	    && Type::convertArrayOrFunctionToPointer(toType)->isPointer()) {
	return llvmBuilder->CreateIntToPtr(reg, ty);
    }
    error::out() << "can not cast '" << fromType << "' to '" << toType
	<< std::endl;
    assert(0);
    return nullptr;
}

ConstVal
cast(ConstVal constVal, const Type *fromType, const Type *toType)
{
    auto reg = llvm::dyn_cast<llvm::Value>(constVal);
    return llvm::dyn_cast<llvm::Constant>(cast(reg, fromType, toType));
}

ConstVal
loadIntConst(const char *val, const Type *type, std::uint8_t radix)
{
    //assureOpenBuildingBlock();

    if (type->isInteger()) {
	auto ty = llvm::APInt(type->getIntegerNumBits(), val, radix);
	return llvm::ConstantInt::get(*llvmContext, ty);
    } else if (type->isPointer() && !std::strcmp(val, "0")) {
	auto ty = llvm::dyn_cast<llvm::PointerType>(TypeMap::get(type));
	return llvm::ConstantPointerNull::get(ty);
    }
    error::out() << "loadConst '" << val << "' of type '" << type << "'"
	<< std::endl;
    assert(0 && "not implemented");
    return nullptr;
}

ConstVal
loadIntConst(std::uint64_t val, const Type *type)
{
    assert(type->isInteger() || (type->isPointer() && val == 0));
    if (type->isPointer() && val == 0) {
	auto ty = llvm::dyn_cast<llvm::PointerType>(TypeMap::get(type));
	return llvm::ConstantPointerNull::get(ty);
    } else if (type->isInteger()) {
	auto ty = llvm::dyn_cast<llvm::IntegerType>(TypeMap::get(type));
	auto isSigned = type->getIntegerKind() ==  Type::SIGNED;
	return llvm::ConstantInt::get(ty, val, isSigned);
    }
    error::out() << "loadConst '" << val << "' of type '" << type << "'"
	<< std::endl;
    assert(0 && "not implemented");
    return nullptr;
}

ConstVal
loadZero(const Type *type)
{
    auto ty = TypeMap::get(type);
    return llvm::Constant::getNullValue(ty);
}

ConstVal
loadConstArray(const std::vector<ConstVal> &val, const Type *type)
{
    assert(type->isArray());
    auto ty = llvm::dyn_cast<llvm::ArrayType>(TypeMap::get(type));
    return llvm::ConstantArray::get(ty, val);
}

ConstVal
loadConstStruct(const std::vector<ConstVal> &val, const Type *type)
{
    assert(type->isStruct());
    auto ty = llvm::dyn_cast<llvm::StructType>(TypeMap::get(type));
    return llvm::ConstantStruct::get(ty, val);
}

//------------------------------------------------------------------------------

Reg
aluInstr(AluOp op, Reg l, Reg r)
{
    assureOpenBuildingBlock();

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

Reg
ptrInc(const Type *type, Reg addr, Reg offset)
{
    assureOpenBuildingBlock();
    auto ty = TypeMap::get(type);

    std::vector<Reg> idxList{1};
    idxList[0] = offset;

    return llvmBuilder->CreateGEP(ty, addr, idxList);
}

Reg
ptrMember(const Type *type, Reg addr, std::size_t index)
{
    assureOpenBuildingBlock();
//    assert(type->isStruct());
    auto ty = TypeMap::get(type);

    std::vector<Reg> idxList{2};
    idxList[0] = loadIntConst(0, Type::getUnsignedInteger(8));
    idxList[1] = loadIntConst(index, Type::getUnsignedInteger(32));

    return llvmBuilder->CreateGEP(ty, addr, idxList);
}

Reg
ptrDiff(const Type *type, Reg addrLeft, Reg addrRight)
{
    assureOpenBuildingBlock();

    auto ty = TypeMap::get(type);
    return llvmBuilder->CreatePtrDiff(ty, addrLeft, addrRight);
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
    if (!setTargetInit) {
	setTarget(codeGenOptLevel);
    }
    assert(targetMachine);

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
