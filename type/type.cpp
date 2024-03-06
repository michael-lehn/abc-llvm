#include <cassert>

#include "type.hpp"
#include "typealias.hpp"

namespace abc {

Type::Type(bool isConst, UStr name)
    : isConst{isConst}, name{name}
{
}

bool
Type::equals(const Type *ty1, const Type *ty2)
{
    if (ty1->isInteger() && ty2->isInteger()) {
	return ty1->isSignedInteger() == ty2->isSignedInteger()
	    && ty1->numBits() == ty2->numBits();
    } else if (ty1->isPointer() && ty2->isPointer()) {
	return equals(ty1->refType(), ty2->refType());
    } else if (ty1->isStruct() && ty2->isStruct()) {
	return ty1->id() == ty2->id();
    }
    return false;
}

const Type *
Type::convert(const Type *from, const Type *to)
{
    if (to->isInteger()) {
	if (from->isInteger()) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isPointer()) {
	if (equals(to->refType(), from->refType())) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isStruct() && equals(to, from)) {
	return to;
    } else {
	return nullptr;
    }
}

UStr
Type::ustr() const
{
    return name;
}

std::size_t
Type::id() const
{
    return isAlias() ? getUnalias()->id() : 0;
}

bool
Type::hasConstFlag() const
{
    return isConst;
}

// for type aliases
bool
Type::isAlias() const
{
    return getUnalias();
}

const Type *
Type::getUnalias() const
{
    return nullptr;
}

const Type *
Type::getAlias(UStr alias) const
{
    return TypeAlias::create(alias, this);
}

const Type *
Type::getAlias(const char *alias) const
{
    return TypeAlias::create(UStr::create(alias), this);
}

// ---

bool
Type::hasSize() const
{
    return isAlias() ? getUnalias()->hasSize() : true;
}

bool
Type::isVoid() const
{
    return isAlias() ? getUnalias()->isVoid() : false;
}

bool
Type::isBool() const
{
    return isAlias() ? getUnalias()->isBool() : false;
}

bool
Type::isNullptr() const
{
    return isAlias() ? getUnalias()->isNullptr() : false;
}


// for integer (sub-)types 
bool
Type::isInteger() const
{
    return isAlias() ? getUnalias()->isInteger() : false;
}

bool
Type::isSignedInteger() const
{
    return isAlias() ? getUnalias()->isSignedInteger() : false;
}

bool
Type::isUnsignedInteger() const
{
    return isAlias() ? getUnalias()->isUnsignedInteger() : false;
}

std::size_t
Type::numBits() const
{
    return isAlias() ? getUnalias()->numBits() : 0;
}

// for pointer and array (sub-)types
bool
Type::isPointer() const
{
    return isAlias() ? getUnalias()->isPointer() : false;
}

bool
Type::isArray() const
{
    return isAlias() ? getUnalias()->isArray() : false;
}

const Type *
Type::refType() const
{
    return isAlias() ? getUnalias()->refType() : nullptr;
}

std::size_t
Type::dim() const
{
    return isAlias() ? getUnalias()->dim() : 0;
}

// for function (sub-)types 
bool
Type::isFunction() const
{
    return isAlias() ? getUnalias()->isFunction() : false;
}

const Type *
Type::retType() const
{
    return isAlias() ? getUnalias()->retType() : nullptr;
}

bool
Type::hasVarg() const
{
    return isAlias() ? getUnalias()->hasVarg() : false;
}

const
std::vector<const Type *> &
Type::paramType() const
{
    static std::vector<const Type *> noArgs;
    return isAlias() ? getUnalias()->paramType() : noArgs;
}

// for enum (sub-)types
bool
Type::isEnum() const
{
    return isAlias() ? getUnalias()->isEnum() : false;
}

const Type *
Type::complete(std::vector<UStr> &&, std::vector<std::int64_t> &&)
{
    if (isAlias() && getUnalias()->isEnum()) {
	assert(0 && "Alias type can not be completed");
    }
    assert(0 && "Type can not be completed");
    return nullptr;
}

// for struct (sub-)types
bool
Type::isStruct() const
{
    return isAlias() ? getUnalias()->isStruct() : false;
}

const Type *
Type::complete(std::vector<UStr> &&, std::vector<const Type *> &&)
{
    if (isAlias() && getUnalias()->isStruct()) {
	assert(0 && "Alias type can not be completed");
    }
    assert(0 && "Type can not be completed");
    return nullptr;
}

const std::vector<UStr> &
Type::memberName() const
{
    static std::vector<UStr> noMembers;
    return isAlias() ? getUnalias()->memberName() : noMembers;
}

const std::vector<const Type *> &
Type::memberType() const
{
    static std::vector<const Type *> noMembers;
    return isAlias() ? getUnalias()->memberType() : noMembers;
}

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    const char *constFlag = type->hasConstFlag() ? "const " : "";

    out << constFlag << type->ustr();
    return out;
}

} // namespace abc
