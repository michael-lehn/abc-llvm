#include <iostream>
#include <set>
#include <sstream>
#include <tuple>

#include "error.hpp"
#include "symtab.hpp"
#include "type.hpp"

/*
 * Inner class: Type::StructData
 */

Type::StructData::StructData(std::size_t id, UStr name)
    : id{id}, name{name}, isComplete{false}, constFlag{false}
{}

Type::StructData::StructData(const StructData &data, bool constFlag)
    : id{data.id}, name{data.name}, isComplete{data.isComplete}
    , constFlag{constFlag}, type{data.type}, ident{data.ident}
{
}

/*
 * Inner class: Type::EnumData
 */

Type::EnumData::EnumData(std::size_t id, UStr name, const Type *intType)
    : id{id}, name{name}, intType{intType}, isComplete{false}, constFlag{false}
{
    assert(intType);
}

Type::EnumData::EnumData(const EnumData &data, bool constFlag)
    : id{data.id}, name{data.name}, intType{data.intType}
    , isComplete{data.isComplete}, constFlag{constFlag}
{
    assert(intType);
}

/*
 * Hidden types: Integer, Pointer, Array, Function, Struct 
 */

//--  Integer class (also misused to represent 'void') -------------------------

struct Integer : public Type
{
    Integer()
	: Type{Type::VOID, IntegerData{0, Type::IntegerKind::UNSIGNED, false}}
    {}

    Integer(std::size_t numBits, IntegerKind kind = IntegerKind::UNSIGNED)
	: Type{Type::INTEGER, IntegerData{numBits, kind, false}}
    {}

    Integer(const IntegerData &data, bool constFlag)
	: Type{Type::INTEGER, IntegerData{data.numBits, data.kind, constFlag}}
    {}

    Integer(const IntegerData &data, UStr aliasIdent)
	: Type{Type::INTEGER, data, aliasIdent}
    {}

    friend bool operator<(const Integer &x, const Integer &y);
};

bool
operator<(const Integer &x, const Integer &y)
{
    const auto &tx = std::tuple{x.aliasIdent.c_str(),
			        x.getIntegerNumBits(),
				x.getIntegerKind(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.aliasIdent.c_str(),
				y.getIntegerNumBits(),
				y.getIntegerKind(),
				y.hasConstFlag()};
    return tx < ty;
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

    Pointer(const PointerData &data,  UStr aliasIdent)
	: Type{Type::POINTER, data, aliasIdent}
    {}
};

bool
operator<(const Pointer &x, const Pointer &y)
{
    const auto &tx = std::tuple{x.getAliasIdent().c_str(),
			       x.isNullPointer(),
			       x.isNullPointer() ? nullptr : x.getRefType(),
			       x.hasConstFlag()};
    const auto &ty = std::tuple{y.getAliasIdent().c_str(),
			       y.isNullPointer(),
			       y.isNullPointer() ? nullptr : y.getRefType(),
			       y.hasConstFlag()};
    return tx < ty;
}

//-- Array class ---------------------------------------------------------------

struct Array : public Type
{
    Array(const Type *refType, std::size_t dim)
	: Type{Type::ARRAY, ArrayData{refType, dim}}
    {}

    Array(const ArrayData data, UStr aliasIdent)
	: Type{Type::ARRAY, data, aliasIdent}
    {}
};

bool
operator<(const Array &x, const Array &y)
{
    const auto &tx = std::tuple{x.getAliasIdent().c_str(),
			       x.getRefType(),
			       x.getDim()};
    const auto &ty = std::tuple{y.getAliasIdent().c_str(),
			       y.getRefType(),
			       y.getDim()};
    return tx < ty;
}

//-- Function class ------------------------------------------------------------

struct Function : public Type
{
    Function(const Type *retType, std::vector<const Type *> paramType,
	     bool hasVarg = false)
	: Type{Type::FUNCTION, FunctionData{retType, paramType, hasVarg}}
    {}

    Function(const FunctionData &data, UStr aliasIdent)
	: Type{Type::FUNCTION, data, aliasIdent}
    {}

    friend bool operator<(const Function &x, const Function &y);
};

bool
operator<(const Function &x, const Function &y)
{
    const auto &tx = std::tuple{x.aliasIdent.c_str(),
				x.getArgType(),
				x.getRetType()};
    const auto &ty = std::tuple{y.aliasIdent.c_str(),
				y.getArgType(),
				y.getRetType()};
    return tx < ty;
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
    }

    bool isComplete()
    {
	assert(std::holds_alternative<StructData>(data));
	return std::get<StructData>(data).isComplete;
    }
};

//--  Enum class -------------------------------------------------------------

struct Enum : public Type
{
    Enum(std::size_t id, UStr name, const Type *intType)
	: Type{Type::ENUM, EnumData{id, name, intType}}
    {}

    Enum(const EnumData &data, bool constFlag)
	: Type{Type::ENUM, EnumData{data, constFlag}}
    {
    }

    Enum(const EnumData &data, UStr aliasIdent)
	: Type{Type::ENUM, data, aliasIdent}
    {
    }

    bool isComplete()
    {
	assert(std::holds_alternative<EnumData>(data));
	return std::get<EnumData>(data).isComplete;
    }
};

//-- Sets for theses types for uniqueness --------------------------------------

static std::set<Integer> *intTypeSet;
static std::set<Pointer> *ptrTypeSet;
static std::set<Array> *arrayTypeSet;
static std::set<Function> *fnTypeSet;
static std::unordered_map<std::size_t, Struct> *structMap;
static std::unordered_map<std::size_t, Struct> *constStructMap;
static std::unordered_map<std::size_t, Enum> *enumMap;
static std::unordered_map<std::size_t, Enum> *constEnumMap;

void
printTypeSet()
{
    if (intTypeSet) {
	std::cerr << "Integer set (size: " << intTypeSet->size() << ")"
	    << std::endl;
	for (const auto &ty: *intTypeSet) {
	    std::cerr << " " << (void *)&ty << ": " << &ty << std::endl;
	}
    }
    if (ptrTypeSet) {
	std::cerr << "Pointer set (size: " << ptrTypeSet->size() << ")"
	    << std::endl;
	for (const auto &ty: *ptrTypeSet) {
	    std::cerr << " " << (void *)&ty << ": " << &ty << std::endl;
	}
    }
    if (arrayTypeSet) {
	std::cerr << "Array set (size: " << arrayTypeSet->size() << ")"
	    << std::endl;
	for (const auto &ty: *arrayTypeSet) {
	    std::cerr << " " << (void *)&ty << ": " << &ty << std::endl;
	}
    }
    if (fnTypeSet) {
	std::cerr << "Function set (size: " << fnTypeSet->size() << ")"
	    << std::endl;
	for (const auto &ty: *fnTypeSet) {
	    std::cerr << " " << (void *)&ty << ": " << &ty << std::endl;
	}
    }
    if (structMap) {
	std::cerr << "Struct map (size: " << structMap->size() << ")"
	    << std::endl;
	for (const auto &ty: *structMap) {
	    std::cerr << " id: " << ty.first << ": " << (void *)&ty.second
		<< std::endl;
	}
    }
    if (constStructMap) {
	std::cerr << "Const struct map (size: " << constStructMap->size() << ")"
	    << std::endl;
	for (const auto &ty: *constStructMap) {
	    std::cerr << " id: " << ty.first << ": " << (void *)&ty.second
		<< std::endl;
	}
    }
    if (enumMap) {
	std::cerr << "Enum map (size: " << enumMap->size() << ")"
	    << std::endl;
	for (const auto &ty: *enumMap) {
	    std::cerr << " id: " << ty.first << ": " << (void *)&ty.second
		<< std::endl;
	}
    }
    if (constEnumMap) {
	std::cerr << "Const enum map (size: " << constEnumMap->size() << ")"
	    << std::endl;
	for (const auto &ty: *constEnumMap) {
	    std::cerr << " id: " << ty.first << ": " << (void *)&ty.second
		<< std::endl;
	}
    }
}

/*
 * Public class Type
 */

//-- Methods -------------------------------------------------------------------

UStr
Type::getAliasIdent() const
{
    return aliasIdent;
}

bool
Type::hasSize() const
{
    if (isVoid()) {
	return false;
    } else if (std::holds_alternative<StructData>(data)) {
	return std::get<StructData>(data).isComplete;
    } else if (std::holds_alternative<EnumData>(data)) {
	return std::get<EnumData>(data).isComplete;
    } else if (isArray()) {
	return getDim();
    } else {
	return true;
    }
}

bool
Type::isVoid() const
{
    return id == VOID;
}

// for integer (sub-)types 

bool
Type::isBool() const
{
    return id == INTEGER && getIntegerNumBits() == 1;
}

bool
Type::hasConstFlag() const
{
    if (std::holds_alternative<IntegerData>(data)) {
	return std::get<IntegerData>(data).constFlag;
    } else if (std::holds_alternative<PointerData>(data)) {
	return std::get<PointerData>(data).constFlag;
    } else if (std::holds_alternative<StructData>(data)) {
	return std::get<StructData>(data).constFlag;
    } else if (std::holds_alternative<EnumData>(data)) {
	return std::get<EnumData>(data).constFlag;
    }
    return false;
}

bool
Type::isInteger() const
{
    return id == INTEGER || id == ENUM;
}

Type::IntegerKind
Type::getIntegerKind() const
{
    assert(std::holds_alternative<IntegerData>(data)
	    || std::holds_alternative<EnumData>(data));

    if (std::holds_alternative<IntegerData>(data)) {
	return std::get<IntegerData>(data).kind;
    } else if (std::holds_alternative<EnumData>(data)) {
	return std::get<EnumData>(data).intType->getIntegerKind();
    } else {
	assert(0);
	return SIGNED;
    }
}

std::size_t
Type::getIntegerNumBits() const
{
    assert(std::holds_alternative<IntegerData>(data)
	    || std::holds_alternative<EnumData>(data));

    if (std::holds_alternative<IntegerData>(data)) {
	return std::get<IntegerData>(data).numBits;
    } else if (std::holds_alternative<EnumData>(data)) {
	assert(std::get<EnumData>(data).intType);
	return std::get<EnumData>(data).intType->getIntegerNumBits();
    } else {
	assert(0);
	return 0;
    }
}

// for pointer and array (sub-)types

bool
Type::isPointer() const
{
    return id == POINTER;
}

bool
Type::isNullPointer() const
{
    if (std::holds_alternative<PointerData>(data)) {
	return std::get<PointerData>(data).isNullptr;
    }
    return false;
}

bool
Type::isArray() const
{
    return id == ARRAY;
}

bool
Type::isArrayOrPointer() const
{
    return isArray() || isPointer();
}

const Type *
Type::getRefType() const
{
    assert(!isNullPointer());
    if (std::holds_alternative<ArrayData>(data)) {
	return std::get<ArrayData>(data).refType;
    }
    assert(std::holds_alternative<PointerData>(data));
    return std::get<PointerData>(data).refType;
}

std::size_t
Type::getDim() const
{
    assert(std::holds_alternative<ArrayData>(data));
    return std::get<ArrayData>(data).dim;
}

// for function (sub-)types 
bool
Type::isFunction() const
{
    return id == FUNCTION;
}

const Type *
Type::getRetType() const
{
    assert(std::holds_alternative<FunctionData>(data));
    return std::get<FunctionData>(data).retType;
}

bool
Type::hasVarg() const
{
    assert(std::holds_alternative<FunctionData>(data));
    return std::get<FunctionData>(data).hasVarg;
}

const std::vector<const Type *> &
Type::getArgType() const
{
    assert(std::holds_alternative<FunctionData>(data));
    return std::get<FunctionData>(data).paramType;
}

// for named type (i.e. struct and enum types)

UStr
Type::getName() const
{
    assert(isStruct() || isEnum());
    if (std::holds_alternative<StructData>(data)) {
	const auto &structData = std::get<StructData>(data);
	return structData.name;
    } else if (std::holds_alternative<EnumData>(data)) {
	const auto &enumData = std::get<EnumData>(data);
	return enumData.name;
    } else {
	assert(0);
	return UStr{};
    }
}

// for struct (sub-)types
bool
Type::isStruct() const
{
    return id == STRUCT;
}

const Type *
Type::complete(const std::vector<UStr> &ident,
	       const std::vector<const Type *> &type)
{
    assert(std::holds_alternative<StructData>(data));
    assert(ident.size() == type.size());
    auto &structData = std::get<StructData>(data);

    if (structData.isComplete) {
	// struct members already defined
	return nullptr;
    }

    structData.isComplete = true;	
    structData.ident = ident;	
    structData.type = type;
    for (std::size_t i = 0; i < structData.ident.size(); ++i) {
	structData.index[structData.ident[i]] = i;
    }

    // complete const version
    auto &constStruct = constStructMap->at(structData.id);
    auto &constStructData = std::get<StructData>(constStruct.data);
    constStructData.name = structData.name;	
    constStructData.isComplete = true;	
    constStructData.ident = structData.ident;
    constStructData.type.resize(structData.type.size());
    for (std::size_t i = 0; i < structData.type.size(); ++i) {
	constStructData.type[i] = getConst(structData.type[i]);
    }
    for (std::size_t i = 0; i < structData.ident.size(); ++i) {
	constStructData.index[structData.ident[i]] = i;
    }
    return this;
}

std::size_t
Type::getNumMembers() const
{
    if (isArray()) {
	return getDim();
    } else if (isStruct()) {
	return getMemberType().size();
    }
    return 1;
}

bool
Type::hasMember(UStr ident) const
{
    assert(std::holds_alternative<StructData>(data));
    const auto &structData = std::get<StructData>(data);
    return structData.index.contains(ident);
}

std::size_t
Type::getMemberIndex(UStr ident) const
{
    assert(std::holds_alternative<StructData>(data));
    const auto &structData = std::get<StructData>(data);
    assert(hasMember(ident));
    return structData.index.at(ident);
}

const Type *
Type::getMemberType(std::size_t index) const
{
    if (std::holds_alternative<StructData>(data)) {
	auto type = getMemberType();
	return index < type.size() ? type.at(index) : nullptr;
    } else if (std::holds_alternative<ArrayData>(data)) {
	auto type = getRefType();
	return index < getDim() ? type : nullptr;
    } else if (index == 0) {
	return this;
    } else {
	return nullptr;
    }
}

const Type *
Type::getMemberType(UStr ident) const
{
    assert(std::holds_alternative<StructData>(data));
    const auto &structData = std::get<StructData>(data);
    assert(hasMember(ident));
    return structData.type[getMemberIndex(ident)];
}

std::vector<const Type *>
Type::getMemberType() const
{
    if (std::holds_alternative<StructData>(data)) {
	const auto &structData = std::get<StructData>(data);
	return structData.type;
    } else if (std::holds_alternative<ArrayData>(data)) {
	return std::vector<const Type *>(getDim(), getRefType());
    } else {
	return std::vector<const Type *>(1, this);
    }
}

const std::vector<UStr> &
Type::getMemberIdent() const
{
    assert(std::holds_alternative<StructData>(data));
    const auto &structData = std::get<StructData>(data);
    return structData.ident;
}

// for enum (sub-)types
bool
Type::isEnum() const
{
    return id == ENUM;
}

const Type *
Type::complete(const std::vector<UStr> &enumIdent,
	       const std::vector<std::int64_t> &enumValue)
{
    assert(std::holds_alternative<EnumData>(data));
    assert(enumIdent.size() == enumValue.size());
    auto &enumData = std::get<EnumData>(data);

    if (enumData.isComplete) {
	// enum constants already defined
	return nullptr;
    }

    enumData.isComplete = true;	
    enumData.enumIdent = std::move(enumIdent);
    enumData.enumValue = std::move(enumValue);

    // complete const version
    auto &constEnum = constEnumMap->at(enumData.id);
    auto &constEnumData = std::get<EnumData>(constEnum.data);

    constEnumData.name = enumData.name;
    constEnumData.intType = getConst(enumData.intType);
    constEnumData.isComplete = true;
    constEnumData.enumIdent = enumData.enumIdent;
    constEnumData.enumValue = enumData.enumValue;
    return this;
}

const std::vector<UStr> &
Type::getEnumIdent() const
{
    assert(std::holds_alternative<EnumData>(data));
    const auto &enumData = std::get<EnumData>(data);
    return enumData.enumIdent;
}

const std::vector<std::int64_t> &
Type::getEnumValue() const
{
    assert(std::holds_alternative<EnumData>(data));
    const auto &enumData = std::get<EnumData>(data);
    return enumData.enumValue;
}

//-- Static functions ----------------------------------------------------------

const Type *
Type::createAlias(UStr aliasIdent, const Type *forType)
{
    if (std::holds_alternative<IntegerData>(forType->data)) {
	const auto &data = std::get<IntegerData>(forType->data);
	return &*intTypeSet->insert(Integer{data, aliasIdent}).first;
    } else if (std::holds_alternative<PointerData>(forType->data)) {
	const auto &data = std::get<PointerData>(forType->data);
	return &*ptrTypeSet->insert(Pointer{data, aliasIdent}).first;
    } else if (std::holds_alternative<ArrayData>(forType->data)) {
	const auto &data = std::get<ArrayData>(forType->data);
	return &*arrayTypeSet->insert(Array{data, aliasIdent}).first;
    } else if (std::holds_alternative<StructData>(forType->data)) {
	auto ty = createIncompleteStruct(aliasIdent);
	ty->data = std::get<StructData>(forType->data);
	return ty;
    } else if (std::holds_alternative<EnumData>(forType->data)) {
	auto ty = createIncompleteEnum(aliasIdent, forType);
	ty->data = std::get<EnumData>(forType->data);
	return ty;
    } else if (std::holds_alternative<FunctionData>(forType->data)) {
	const auto &data = std::get<FunctionData>(forType->data);
	return &*fnTypeSet->insert(Function{data, aliasIdent}).first;
    }
    assert(0);
    return nullptr;
}

const Type *
Type::getConst(const Type *type)
{
    if (std::holds_alternative<IntegerData>(type->data)) {
	const auto &data = std::get<IntegerData>(type->data);
	return &*intTypeSet->insert(Integer{data, true}).first;
    } else if (std::holds_alternative<PointerData>(type->data)) {
	const auto &data = std::get<PointerData>(type->data);
	return &*ptrTypeSet->insert(Pointer{data, true}).first;
    } else if (type->isArray()) {
	return getArray(getConst(type->getRefType()), type->getDim());
    } else if (std::holds_alternative<StructData>(type->data)) {
	const auto &data = std::get<StructData>(type->data);
	return &constStructMap->at(data.id);
    } else if (std::holds_alternative<EnumData>(type->data)) {
	const auto &data = std::get<EnumData>(type->data);
	return &constEnumMap->at(data.id);
    }
    return nullptr;
}

const Type *
Type::getConstRemoved(const Type *type)
{
    if (std::holds_alternative<IntegerData>(type->data)) {
	const auto &data = std::get<IntegerData>(type->data);
	return &*intTypeSet->insert(Integer{data, false}).first;
    } else if (std::holds_alternative<PointerData>(type->data)) {
	const auto &data = std::get<PointerData>(type->data);
	return &*ptrTypeSet->insert(Pointer{data, false}).first;
    } else if (std::holds_alternative<StructData>(type->data)) {
	const auto &data = std::get<StructData>(type->data);
	return &structMap->at(data.id);
    } else if (std::holds_alternative<EnumData>(type->data)) {
	const auto &data = std::get<EnumData>(type->data);
	return &enumMap->at(data.id);
    }
    return type;
}

// Create integer/void type or return existing type

const Type *
Type::getVoid()
{
    if (!intTypeSet) {
	intTypeSet = new std::set<Integer>;
    }
    return &*intTypeSet->insert(Integer{}).first;
}

const Type *
Type::getChar()
{
    if (std::numeric_limits<char>::is_signed) {
	return getSignedInteger(8);
    }
    return getUnsignedInteger(8);
}

const Type *
Type::getString(std::size_t len)
{
    return getArray(getChar(), len + 1);
}


const Type *
Type::getBool()
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
Type::getNullPointer()
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
Type::getFunction(const Type *retType, std::vector<const Type *> paramType,
		  bool hasVarg)
{
    if (!fnTypeSet) {
	fnTypeSet = new std::set<Function>;
    }
    return &*fnTypeSet->insert(Function{retType, paramType, hasVarg}).first;
}

// create structured types

Type *
Type::createIncompleteStruct(UStr name)
{
    assert(!constStructMap == !structMap);
    if (!structMap) {
	structMap = new std::unordered_map<std::size_t, Struct>;
	constStructMap = new std::unordered_map<std::size_t, Struct>;
    }
    if (auto ty = Symtab::getNamedType(name, Symtab::CurrentScope)) {
	if (!ty->isStruct()) {
	    return nullptr;
	}
	assert(std::holds_alternative<StructData>(ty->data));
	auto &structData = std::get<StructData>(ty->data);
	return &structMap->at(structData.id);
    }

    // new struct type is needed
    static size_t id;
    auto ty = &structMap->insert({id, Struct{id, name}}).first->second;
    // also create the const version
    auto data = std::get<StructData>(ty->data);
    constStructMap->insert({id, Struct{data, true}});
    ++id;

    // add type to current scope
    auto tyAdded = Symtab::addTypeAlias(name, ty);
    assert(ty == tyAdded);
    return ty;
}

// create enumeration types

Type *
Type::createIncompleteEnum(UStr name, const Type *intType)
{
    assert(!constEnumMap == !enumMap);
    if (!enumMap) {
	enumMap = new std::unordered_map<std::size_t, Enum>;
	constEnumMap = new std::unordered_map<std::size_t, Enum>;
    }
    if (auto ty = Symtab::getNamedType(name, Symtab::CurrentScope)) {
	if (!ty->isEnum()) {
	    return nullptr;
	}
	assert(std::holds_alternative<EnumData>(ty->data));
	auto &enumData = std::get<EnumData>(ty->data);
	return &enumMap->at(enumData.id);
    }

    // new enum type is needed
    static size_t id;
    auto ty = &enumMap->insert({id, Enum{id, name, intType}}).first->second;
    // also create the const version
    auto data = std::get<EnumData>(ty->data);
    constEnumMap->insert({id, Enum{data, true}});
    ++id;

    // add type to current scope
    auto tyAdded = Symtab::addTypeAlias(name, ty);
    assert(ty == tyAdded);
    return ty;
}

/*
 * Type casts
 */

const Type *
Type::getTypeConversion(const Type *from, const Type *to, Token::Loc loc,
			bool silent, bool allowConstCast)
{
    if (from == to) {
	return to;
    } else if (from->isVoid() && !to->isVoid()) {
	return nullptr;
    } else if (from->isNullPointer() && to->isPointer()) {
	return from;
    } else if (from->isPointer() && to->isPointer()) {
	auto fromRefTy = Type::getConstRemoved(from->getRefType());
	auto toRefTy = Type::getConstRemoved(to->getRefType());
	bool refTyCheck = *fromRefTy == *toRefTy;
	bool constCheck = to->getRefType()->hasConstFlag()
		       || !from->getRefType()->hasConstFlag();

	if (!refTyCheck && !fromRefTy->isVoid() && !toRefTy->isVoid()) {
	    if (!silent) {
		error::out() << loc << ": error: casting '" << from
		    << "' to '" << to << "'" << std::endl;
		error::fatal();
	    }
	    return nullptr;
	}
	if (!constCheck && !allowConstCast) {
	    if (!silent) {
		error::out() << loc << ": error: casting '" << from
		    << "' to '" << to << "' discards const qualifier"
		    << std::endl;
		error::fatal();
	    }
	    return nullptr;
	}
	return from;
    } else if (getConstRemoved(from) == getConstRemoved(to)) {
	return to;
    } else if (from->isInteger() && to->isInteger()) {
	return to;
    } else if (from->isArray() && to->isPointer()) {
	auto fromRefTy = Type::getConstRemoved(from->getRefType());
	auto toRefTy = Type::getConstRemoved(to->getRefType());
	bool refTyCheck = *fromRefTy == *toRefTy;

	if (!refTyCheck && !toRefTy->isVoid()) {
	    if (!silent) {
		error::out() << loc << ": error: casting '" << from
		    << "' to '" << to << "'" << std::endl;
		error::fatal();
	    }
	    return nullptr;
	}
	return to;
    } else if (from->isPointer() && to->isBool()) {
	return to;
    } else if (from->isPointer() && to->isArray()) {
	return nullptr;
    } else if (from->isArray() && to->isArray()) {
	auto fromRefTy = Type::getConstRemoved(from->getRefType());
	auto toRefTy = Type::getConstRemoved(to->getRefType());
	bool refTyCheck = *fromRefTy == *toRefTy;

	if (!refTyCheck || from->getDim() != to->getDim()) {
	    return nullptr;
	}
	return from;
    } else if (from->isFunction() && to->isPointer()) {
	return from;
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

//-- Compare types -------------------------------------------------------------
bool
operator==(const Type &x, const Type &y)
{
    if (x.id != y.id) {
	return false;
    }
    if (x.isVoid() && y.isVoid()) {
	return true;
    } else if (x.isInteger() && y.isInteger()) {
	return x.getIntegerKind() == y.getIntegerKind()
	    && x.getIntegerNumBits() == y.getIntegerNumBits()
	    && x.hasConstFlag() == y.hasConstFlag();
    } else if (x.isNullPointer() && y.isNullPointer()) {
	return true;
    } else if (x.isNullPointer() || y.isNullPointer()) {
	return false;
    } else if (x.isPointer() && y.isPointer()) {
	const auto &xRef = *x.getRefType();
	const auto &yRef = *y.getRefType();
	return xRef == yRef;
    } else if (x.isArray() && y.isArray()) {
	const auto &xRef = *x.getRefType();
	const auto &yRef = *y.getRefType();
	return x.getDim() == y.getDim() && xRef == yRef;
    } else if (x.isFunction() && y.isFunction()) {
	const auto &xRet = *x.getRetType();
	const auto &yRet = *y.getRetType();
	if (xRet != yRet) {
	    return false;
	}
	if (x.getArgType().size() != y.getArgType().size()) {
	    return false;
	}
	for (std::size_t i = 0; i < x.getArgType().size(); ++i) {
	    if (*x.getArgType()[i] != *y.getArgType()[i]) {
		return false;
	    }
	}
	return true;
    }
    return &x == &y;
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
    if (type->getAliasIdent().c_str()) {
	out << type->getAliasIdent().c_str() << " (aka '";
	if (type->hasConstFlag()) {
	    out << "const ";
	}
    }

    if (type->isVoid()) {
	out << "void";
    } else if (type->isBool()) {
	out << "bool";
    } else if (type->isEnum()) {
	out << type->getName().c_str();
	if (!type->hasSize()) {
	    out << " (incomplete enum)";
	}
    } else if (type->isInteger()) {
	out << (type->getIntegerKind() == Type::SIGNED ? "i" : "u")
	    << type->getIntegerNumBits();
    } else if (type->isNullPointer()) {
	    out << "-> NULL";
    } else if (type->isPointer()) {
	auto refTy = type->getRefType();
	if (refTy->isStruct()) {
	    out << "-> ";
	    if (refTy->hasConstFlag()) {
		out << "const ";
	    }
	    out << refTy->getName().c_str();
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
	out << type->getName().c_str();
	if (!type->hasSize()) {
	    out << " (incomplete struct)";
	}
	/*
	type = Type::getConstRemoved(type);
	auto memType = type->getMemberType();
	auto memIdent = type->getMemberIdent();
	out << "struct " << type->getName().c_str();
	if (type->hasSize()) {
	    out << "{";
	    for (std::size_t i = 0; i < memType.size(); ++i) {
		out << memIdent[i] << ": " << memType[i];
		if (i + 1 < memType.size()) {
		    out << ", ";
		}
	    }
	    out << "}";
	}
	*/
    } else {
	out << "unknown type: id = " << type->id
	    << ", address = " << (int *)type
	    << std::endl;
	error::fatal();
    }
    if (type->getAliasIdent().c_str()) {
	out << "')";
    }
    return out;
}
