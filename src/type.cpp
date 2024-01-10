#include <iostream>
#include <set>

#include "type.hpp"

//------------------------------------------------------------------------------


struct Integer : public Type
{
    Integer(std::size_t numBits, IntegerKind kind)
	: Type{Type::INTEGER, IntegerData{numBits, kind}}
    {}

    friend bool operator<(const Integer &x, const Integer &y);
};

bool
operator<(const Integer &x, const Integer &y)
{
    return x.getIntegerKind() < y.getIntegerKind()
	|| x.getIntegerKind() == y.getIntegerKind()
	    && x.getIntegerNumBits() < y.getIntegerNumBits();
}

static std::set<Integer> *intTypeSet;

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

//------------------------------------------------------------------------------

struct Pointer : public Type
{
    Pointer(const Type *refType) : Type{Type::POINTER, PointerData{refType}} {}
};

bool
operator<(const Pointer &x, const Pointer &y)
{
    return x.getRefType() < y.getRefType();
}


static std::set<Pointer> *ptrTypeSet;

const Type *
Type::getPointer(const Type *refType)
{
    if (!ptrTypeSet) {
	ptrTypeSet = new std::set<Pointer>;
    }
    return &*ptrTypeSet->insert(Pointer{refType}).first;
}

//------------------------------------------------------------------------------

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

static std::set<Function> *fnTypeSet;

const Type *
Type::getFunction(const Type *retType, std::vector<const Type *> argType)
{
    if (!fnTypeSet) {
	fnTypeSet = new std::set<Function>;
    }
    return &*fnTypeSet->insert(Function{retType, argType}).first;
}

//------------------------------------------------------------------------------

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    if (!type) {
	out << "void";
    } else if (type->isInteger()) {
	out << (type->getIntegerKind() == Type::SIGNED ? "i" : "u")
	    << type->getIntegerNumBits();
    } else if (type->isPointer()) {
	out << "pointer to " << type->getRefType();
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
