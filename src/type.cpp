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
	: Type{Type::VOID, IntegerData{0, Type::IntegerKind::UNSIGNED, true}}
    {}

    Integer(std::size_t numBits, IntegerKind kind = IntegerKind::UNSIGNED)
	: Type{Type::INTEGER, IntegerData{numBits, kind, false}}
    {}

    Integer(const IntegerData &data, bool constFlag)
	: Type{Type::INTEGER, IntegerData{data.numBits, data.kind, constFlag}}
    {}

    friend bool operator<(const Integer &x, const Integer &y);
};

bool
operator<(const Integer &x, const Integer &y)
{
    return x.getIntegerKind() < y.getIntegerKind()
	|| (x.getIntegerKind() == y.getIntegerKind()
	    && x.getIntegerNumBits() < y.getIntegerNumBits())
	|| (x.getIntegerKind() == y.getIntegerKind()
	    && x.getIntegerNumBits() == y.getIntegerNumBits()
	    && x.hasConstFlag() < y.hasConstFlag());
}

//-- Pointer class -------------------------------------------------------------

struct Pointer : public Type
{
    Pointer(const Type *refType, bool isNullptr)
	: Type{Type::POINTER, PointerData{refType, isNullptr, false}}
    {}

    Pointer(const PointerData &data,  bool constFlag)
	: Type{Type::POINTER,
	       PointerData{data.refType, data.isNullptr, constFlag}}
    {}
};

bool
operator<(const Pointer &x, const Pointer &y)
{
    return (x.isNullPointer() && !y.isNullPointer())
	|| (!x.isNullPointer() && !y.isNullPointer()
	    && x.getRefType() < y.getRefType())
	|| (!x.isNullPointer() && !y.isNullPointer()
	    && x.getRefType() == y.getRefType()
	    && x.hasConstFlag() < y.hasConstFlag());
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
    Function(const Type *retType, std::vector<const Type *> argType,
	     bool hasVarg = false)
	: Type{Type::FUNCTION, FunctionData{retType, argType, hasVarg}}
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
    Struct(std::size_t id, UStr name)
	: Type{Type::STRUCT, StructData{id, name}}
    {}

    Struct(const StructData &data, bool constFlag)
	: Type{Type::STRUCT, StructData{data, constFlag}}
    {
	assert(!constFlag &&
		"internal error: Only copy struct data when constFlag is set");
    }

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
static std::unordered_map<std::size_t, Struct> *structMap;

//-- Static functions ----------------------------------------------------------

// Create integer/void type or return existing type

const Type *
Type::getConst(const Type *type)
{
    if (type->isInteger()) {
	const auto &data = std::get<IntegerData>(type->data);
	return &*intTypeSet->insert(Integer{data, true}).first;
    } else if (type->isPointer()) {
	const auto &data = std::get<PointerData>(type->data);
	return &*ptrTypeSet->insert(Pointer{data, true}).first;
    } else if (type->isArray()) {
	return getArray(getConst(type->getRefType()), type->getDim());
    } else if (type->isStruct()) {
	const auto &data = std::get<StructData>(type->data);
	std::string name = ".const_" + std::string{data.name.c_str()};
	auto ty = createIncompleteStruct(UStr{name});
	std::vector<const Type *> memConstType{data.type.size()};
	for (std::size_t i = 0; i < data.type.size(); ++ i) {
	    memConstType[i] = getConst(data.type.at(i));
	}
	ty->complete(std::vector<const char *>{data.ident},
		     std::move(memConstType));
	auto &tyData = std::get<StructData>(ty->data);
	tyData.constFlag = true;
	tyData.name = data.name;
	return ty;
    }
    return nullptr;
}

const Type *
Type::getConstRemoved(const Type *type)
{
    if (type->isInteger()) {
	const auto &data = std::get<IntegerData>(type->data);
	return &*intTypeSet->insert(Integer{data, false}).first;
    } else if (type->isPointer()) {
	const auto &data = std::get<PointerData>(type->data);
	return &*ptrTypeSet->insert(Pointer{data, false}).first;
    } else if (type->isStruct()) {
	auto name = type->getName();
	auto  ty = Symtab::getNamedType(name, Symtab::AnyScope);
	assert(ty);
	return ty;
    }
    return type;
}

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
Type::getFunction(const Type *retType, std::vector<const Type *> argType,
		  bool hasVarg)
{
    if (!fnTypeSet) {
	fnTypeSet = new std::set<Function>;
    }
    return &*fnTypeSet->insert(Function{retType, argType, hasVarg}).first;
}

// create strcutured types

Type *
Type::createIncompleteStruct(UStr name)
{
    if (!structMap) {
	structMap = new std::unordered_map<std::size_t, Struct>;
    }
    if (auto ty = Symtab::getNamedType(name, Symtab::CurrentScope)) {
	if (!ty->isStruct()) {
	    return nullptr;
	}
	auto &structData = std::get<StructData>(ty->data);
	// type already exists (not a problem if it's already complete)
	return &structMap->at(structData.id);
    }

    // new struct type is needed
    static size_t id;
    auto ty = &structMap->insert({id, Struct{id, name}}).first->second;
    ++id;

    // add type to current scope
    auto tyAdded = Symtab::addNamedType(name, ty);
    assert(ty == tyAdded);
    return ty;
}

void
Type::deleteStruct(const Type *ty)
{
    assert(structMap
	    && "Type::deleteStruct() called before any struct was created");
    
    assert(ty->isStruct());
    const auto &structData = std::get<StructData>(ty->data);
    auto numErased = structMap->erase(structData.id);
    assert(numErased == 1 && "Type::deleteStruct(): no struct deleted");
}

/*
 * Type casts
 */

const Type *
Type::getTypeConversion(const Type *from, const Type *to, Token::Loc loc)
{
    if (from == to) {
	return to;
    } else if (getConstRemoved(from) == getConstRemoved(to)) {
	if (to->hasConstFlag() && !from->hasConstFlag()) {
	    error::out() << loc << ": warning: casting '" << from
		<< "' to '" << to << "' discards const qualifier" << std::endl;
	}
	return to;
    } else if (from->isInteger() && to->isInteger()) {
	// TODO: -Wconversion generate warning if sizeof(to) < sizeof(from)
	return to;
    } else if (from->isNullPointer() && to->isPointer()) {
	return from;
    } else if (from->isArrayOrPointer() && to->isPointer()) {
	auto fromRefTy = Type::getConstRemoved(from->getRefType());
	auto toRefTy = Type::getConstRemoved(to->getRefType());

	if (fromRefTy != toRefTy && !fromRefTy->isVoid() && !toRefTy->isVoid())
	{
	    error::out() << loc << ": warning: casting '" << from
		<< "' to '" << to << "'" << std::endl;
	}
	if (!to->getRefType()->hasConstFlag()
		&& from->getRefType()->hasConstFlag()) {
	    error::out() << loc << ": warning: casting '" << from
		<< "' to '" << to << "' discards const qualifier" << std::endl;
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
	return out;
    }

    if (type->hasConstFlag()) {
	out << "const ";
    }

    if (type->isVoid()) {
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
	if (type->hasVarg()) {
	    out << ", ...";
	}
	out << "): " << type->getRetType();
    } else if (type->isStruct()) {
	type = Type::getConstRemoved(type);
	auto memType = type->getMemberType();
	auto memIdent = type->getMemberIdent();
	out << "struct " << type->getName().c_str();
	if (type->hasSize()) {
	    out << "{";
	    for (std::size_t i = 0; i < memType.size(); ++i) {
		out << memIdent[i] << ":" << memType[i];
		if (i + 1 < memType.size()) {
		    out << ", ";
		}
	    }
	    out << "}";
	}
    } else {
	out << "unknown type: id = " << type->id
	    << ", address = " << (int *)type
	    << std::endl;
	error::fatal();
    }
    return out;
}
