#include <iostream>
#include <set>
#include <sstream>

#include "error.hpp"
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
    Pointer(const Type *refType) : Type{Type::POINTER, PointerData{refType}} {}
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
    Struct(StructData &&structData)
	: Type{Type::STRUCT, StructData{structData}}
    {}

    const StructData &getData(void) const
    {
	return std::get<StructData>(data);
    }
};

bool
operator<(const Struct &x, const Struct &y)
{
    const auto &xType = x.getData().type;
    const auto &yType = y.getData().type;
    const auto &xIdent = x.getData().ident;
    const auto &yIdent = y.getData().ident;

    if (xType.size() < yType.size()) {
	return true;
    }
    for (std::size_t i = 0; i < xType.size(); ++i) {
	if (xType[i] < yType[i]) {
	    return true;
	}
	if (xIdent[i] < yIdent[i]) {
	    return true;
	}
    }
    return false;
}

//-- Sets for theses types for uniqueness --------------------------------------

static std::set<Integer> *intTypeSet;
static std::set<Pointer> *ptrTypeSet;
static std::set<Array> *arrayTypeSet;
static std::set<Function> *fnTypeSet;
static std::unordered_map<const char *, const Type *> *aliasSet;
static std::set<Struct> *structSet;

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
    return &*ptrTypeSet->insert(Pointer{refType}).first;
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

/*
 * Named types
 */

void
Type::createAlias(const char *name, const Type *type)
{
    if (!aliasSet) {
	aliasSet = new std::unordered_map<const char *, const Type *>;
    }
    aliasSet->insert({name, type});
}

const Type *
Type::getNamed(UStr name)
{
    if (!aliasSet) {
	aliasSet = new std::unordered_map<const char *, const Type *>;
    }
    if (!aliasSet->contains(name.c_str())) {
	return nullptr;
    }
    return aliasSet->find(name.c_str())->second;
}

const Type *
Type::createStruct(const char *name, const std::vector<const char *> &ident,
		   const std::vector<const Type *> &type)
{
    if (!structSet) {
	structSet = new std::set<Struct>;
    }
    assert(!name || !getNamed(name));

    Type::StructData structData;
    structData.type = type;
    structData.ident = ident;

    for (std::size_t i = 0; i < ident.size(); ++i) {
	structData.index[ident[i]] = i;
    }
    auto ty = &*structSet->insert(StructData{std::move(structData)}).first;

    if (name) {
	createAlias(name, ty);
    }
    return ty;
}

/*
 * Type information and casts
 */

std::size_t
Type::getSizeOf(const Type *type)
{
    if (type->isInteger()) {
	return type->getIntegerNumBits() / 8;
    } else if (type->isPointer() || type->isFunction()) {
	error::out() << "Warning: Currently pointers and addresses are"
	    " assumed to be 64 bits" << std::endl;
	return 8;
    } else if (type->isArray()) {
	return type->getDim() * getSizeOf(type->getRefType());
    }
    assert(0 && "getSizeOf not implemented for this type");
    return 0;
}

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
	out << "-> " << type->getRefType();
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
