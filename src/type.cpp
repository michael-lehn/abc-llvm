#include <iostream>
#include <set>
#include <sstream>

#include "error.hpp"
#include "symtab.hpp"
#include "type.hpp"

//--  Integer class (also misused to represent 'void') -------------------------

struct Integer : public Type
{
    Integer()
	: Type{Type::VOID, IntegerData{0, Type::IntegerKind::UNSIGNED}}
    {}

    Integer(std::size_t numBits, IntegerKind kind = IntegerKind::UNSIGNED)
	: Type{Type::INTEGER, IntegerData{numBits, kind}}
    {}

    friend bool operator<(const Integer &x, const Integer &y);
};

bool
operator<(const Integer &x, const Integer &y)
{
    return x.getIntegerKind() < y.getIntegerKind()
	|| (x.getIntegerKind() == y.getIntegerKind()
	    && x.getIntegerNumBits() < y.getIntegerNumBits());
}

//-- Pointer class -------------------------------------------------------------

struct Pointer : public Type
{
    Pointer(const Type *refType, bool isNullptr)
	: Type{Type::POINTER, PointerData{refType, isNullptr}}
    {}
};

bool
operator<(const Pointer &x, const Pointer &y)
{
    return x.getRefType() < y.getRefType();
}

//-- Array class ---------------------------------------------------------------

struct Array : public Type
{
    Array(const Type *refType, std::size_t dim)
	: Type{Type::ARRAY, ArrayData{refType, dim}}
    {}
};

bool
operator<(const Array &x, const Array &y)
{
    return x.getRefType() < y.getRefType()
	|| (x.getRefType() == y.getRefType() && x.getDim() < y.getDim());
}

//-- Function class ------------------------------------------------------------

struct Function : public Type
{
    Function(const Type *retType, std::vector<const Type *> argType)
	: Type{Type::FUNCTION, FunctionData{retType, argType}}
    {}
};

bool
operator<(const Function &x, const Function &y)
{
    auto xArg = x.getArgType();
    auto yArg = y.getArgType();

    if (x.getRetType() < y.getRetType() || xArg.size() < yArg.size()) {
	return true;
    }
    for (std::size_t i = 0; i < yArg.size(); ++ i) {
	if (xArg[i] < yArg[i]) {
	    return true;
	}
    }
    return false;
}

//--  Struct class -------------------------------------------------------------

struct Struct : public Type
{
    Struct(void) : Type{Type::STRUCT, StructData{}} {}

    bool isComplete(void)
    {
	return std::get<StructData>(data).isComplete;
    }
};

//-- Sets for theses types for uniqueness --------------------------------------

static std::set<Integer> *intTypeSet;
static std::set<Pointer> *ptrTypeSet;
static std::set<Array> *arrayTypeSet;
static std::set<Function> *fnTypeSet;
static std::unordered_map<UStr, Struct> *structMap;

//-- Static functions ----------------------------------------------------------

// Create integer/void type or return existing type

const Type *
Type::getVoid(void)
{
    if (!intTypeSet) {
	intTypeSet = new std::set<Integer>;
    }
    return &*intTypeSet->insert(Integer{}).first;
}

const Type *
Type::getBool(void)
{
    if (!intTypeSet) {
	intTypeSet = new std::set<Integer>;
    }
    return &*intTypeSet->insert(Integer{1}).first;
}

static const Type *
getInteger(std::size_t numBits, Type::IntegerKind kind)
{
    if (!intTypeSet) {
	intTypeSet = new std::set<Integer>;
    }
    return &*intTypeSet->insert(Integer{numBits, kind}).first;
}

const Type *
Type::getUnsignedInteger(std::size_t numBits)
{
    return getInteger(numBits, Type::IntegerKind::UNSIGNED);
}

const Type *
Type::getSignedInteger(std::size_t numBits)
{
    return getInteger(numBits, Type::IntegerKind::SIGNED);
}

// Create pointer type or return existing type

const Type *
Type::getPointer(const Type *refType)
{
    if (!ptrTypeSet) {
	ptrTypeSet = new std::set<Pointer>;
    }
    return &*ptrTypeSet->insert(Pointer{refType, false}).first;
}

const Type *
Type::getNullPointer(void)
{
    if (!ptrTypeSet) {
	ptrTypeSet = new std::set<Pointer>;
    }
    return &*ptrTypeSet->insert(Pointer{nullptr, true}).first;
}

// Create array type or return existing type

const Type *
Type::getArray(const Type *refType, std::size_t dim)
{
    if (!arrayTypeSet) {
	arrayTypeSet = new std::set<Array>;
    }
    return &*arrayTypeSet->insert(Array{refType, dim}).first;
}

// Create function type or return existing type

const Type *
Type::getFunction(const Type *retType, std::vector<const Type *> argType)
{
    if (!fnTypeSet) {
	fnTypeSet = new std::set<Function>;
    }
    return &*fnTypeSet->insert(Function{retType, argType}).first;
}

// create strcutured types

Type *
Type::createIncompleteStruct(UStr ident)
{
    if (!structMap) {
	structMap = new std::unordered_map<UStr, Struct>;
    }
    bool add = !structMap->contains(ident);
    auto ty = &structMap->insert({ident, Struct{}}).first->second;
    if (add) {
	Symtab::createTypeAlias(ident, ty);
    }
    return ty;
}

void
Type::deleteStruct(UStr ident)
{
    assert(structMap
	    && "Type::deleteStruct() called before any struct was created");
    assert(structMap->erase(ident) == 1
	    && "Type::deleteStruct(): no struct deleted");
}

/*
 * Type casts
 */

const Type *
Type::getTypeConversion(const Type *from, const Type *to, Token::Loc loc)
{
    if (from == to) {
	return to;
    } else if (from->isInteger() && to->isInteger()) {
	// TODO: -Wconversion generate warning if sizeof(to) < sizeof(from)
	return to;
    } else if (from->isArrayOrPointer() && to->isPointer()) {
	if (from->getRefType() != to->getRefType()
	    && !to->getRefType()->isVoid()
	    && !from->getRefType()->isVoid()) {
	    error::out() << loc << ": warning: casting '" << from
		<< "' to '" << to << "'" << std::endl;
	}
	return from;
    } else if (from->isFunction() && to->isPointer()) {
	if (from != to->getRefType() && !to->getRefType()->isVoid()) {
	    error::out() << loc << ": warning: casting '" << from
		<< "' to '" << to << "'" << std::endl;
	}
	return from; // no cast required
    } else if (convertArrayOrFunctionToPointer(from)->isPointer()
	    && to->isInteger()) {
	error::out() << loc << ": warning: casting '" << from
	    << "' to '" << to << "'" << std::endl;
	return to;
    } else if (from->isInteger()
	    && convertArrayOrFunctionToPointer(to)->isPointer()) {
	error::out() << loc << ": warning: casting '" << from
	    << "' to '" << to << "'" << std::endl;
	return to;
    }
    return nullptr;
}

const Type *
Type::convertArrayOrFunctionToPointer(const Type *ty)
{
    if (ty->isArray()) {
	return getPointer(ty->getRefType());
    } else if (ty->isFunction()) {
	return getPointer(ty);
    }
    return ty;
}

//-- Print type ----------------------------------------------------------------

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    if (!type) {
	out << "illegal";
    } else if (type->isVoid()) {
	out << "void";
    } else if (type->isInteger()) {
	out << (type->getIntegerKind() == Type::SIGNED ? "i" : "u")
	    << type->getIntegerNumBits();
    } else if (type->isPointer()) {
	auto refTy = type->getRefType();
	if (refTy->isStruct()) {
	    out << "-> struct{..}";
	} else {
	    out << "-> " << refTy;
	}
    } else if (type->isArray()) {
	out << "array[" << type->getDim() << "] of " << type->getRefType();
    } else if (type->isFunction()) {
	out << "fn(";
	auto arg = type->getArgType();
	for (std::size_t i = 0; i < arg.size(); ++i) {
	    out << ": " << arg[i];
	    if (i + 1 != arg.size()) {
		out << ",";
	    }
	}
	out << "): " << type->getRetType();
    } else if (type->isStruct()) {
	auto memType = type->getMemberType();
	auto memIdent = type->getMemberIdent();
	out << "{";
	for (std::size_t i = 0; i < memType.size(); ++i) {
	    out << memIdent[i] << ":" << memType[i];
	    if (i + 1 < memType.size()) {
		out << ", ";
	    }
	}
	out << "}";
    } else {
	out << "unknown type: id = " << type->id
	    << ", address = " << (int *)type
	    << std::endl;
	error::fatal();
    }
    return out;
}
