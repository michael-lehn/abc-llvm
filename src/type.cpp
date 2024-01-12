#include <iostream>
#include <set>

#include "type.hpp"

//--  Integer class (also misused to represent 'void') -------------------------

struct Integer : public Type
{
    Integer()
	: Type{Type::VOID, IntegerData{0, Type::IntegerKind::UNSIGNED}}
    {}

    Integer(std::size_t numBits, IntegerKind kind)
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

//-- Sets for theses types for uniqueness --------------------------------------

static std::set<Integer> *intTypeSet;
static std::set<Pointer> *ptrTypeSet;
static std::set<Array> *arrayTypeSet;
static std::set<Function> *fnTypeSet;

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
	out << "pointer to " << type->getRefType();
    } else if (type->isArray()) {
	out << "array[" << type->getDim() << "] of " << type->getRefType();
    } else if (type->isFunction()) {
	out << "fn(";
	auto arg = type->getArgType();
	for (std::size_t i = 0; i < arg.size(); ++i) {
	    out << " :" << arg[i];
	    if (i + 1 != arg.size()) {
		out << ",";
	    }
	}
	out << ") :" << type->getRetType();
    }
    return out;
}
