#include <algorithm>
#include <cassert>
#include <iostream>

#include "integertype.hpp"
#include "pointertype.hpp"
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
    if (ty1->hasConstFlag() != ty2->hasConstFlag()) {
	return false;
    }
    if (ty1->isVoid() && ty2->isVoid()) {
	return true;
    } else if (ty1->isInteger() && ty2->isInteger()) {
	return ty1->isSignedInteger() == ty2->isSignedInteger()
	    && ty1->numBits() == ty2->numBits();
    } else if (ty1->isFloatType() && ty2->isFloatType()) {
	return (ty1->isFloat() && ty2->isFloat())
	    || (ty1->isDouble() && ty2->isDouble());
    } else if (ty1->isPointer() && ty2->isPointer()) {
	if (ty1->isNullptr() || ty2->isNullptr()) {
	    return ty1->isNullptr() == ty2->isNullptr();
	} else {
	    return equals(ty1->refType(), ty2->refType());
	}
    } else if (ty1->isStruct() && ty2->isStruct()) {
	return ty1->id() == ty2->id();
    } else if (ty1->isArray() && ty2->isArray()) {
	return ty1->dim() == ty2->dim()
	    && equals(ty1->refType(), ty2->refType());
    } else if (ty1->isFunction() && ty2->isFunction()) {
	if (!equals(ty1->retType(), ty2->retType())) {
	    return false;
	}
	if (ty1->hasVarg() != ty2->hasVarg()) {
	    return false;
	}
	const auto &paramType1 = ty1->paramType();
	const auto &paramType2 = ty2->paramType();
	if (paramType1.size() != paramType2.size()) {
	    return false;
	}
	for (std::size_t i = 0; i < paramType1.size(); ++i) {
	    if (!equals(paramType1[i], paramType2[i])) {
		return false;
	    }
	}
	return true;
    }
    return false;
}

const Type *
Type::common(const Type *ty1, const Type *ty2)
{
    assert(ty1);
    assert(ty2);

    const Type *common = nullptr;
    if (equals(ty1->getConstRemoved(), ty2->getConstRemoved())) {
	common = ty1;
    } else if (ty1->isArray() && ty2->isArray()) {
	// both types are arrays but refernce type or dimension are different
	if (equals(ty1->refType(), ty2->refType())) {
	    common = PointerType::create(ty1->refType());
	}
    } else if (ty1->isInteger() && ty2->isInteger()) {
	auto size = std::max(ty1->numBits(), ty2->numBits());
	if (ty1->isUnsignedInteger() || ty2->isUnsignedInteger()) {
	    common = IntegerType::createUnsigned(size);
	} else {
	    common = IntegerType::createSigned(size);
	}
    }
    if (common && (ty1->hasConstFlag() || ty2->hasConstFlag())) {
	common->getConst();
    }
    return common;
}

const Type *
Type::convert(const Type *from, const Type *to)
{
    if (equals(from, to)) {
	return to;
    } else if (to->isBool()) {
	if (from->isInteger()) {
	    return to;
	} else if (from->isPointer()) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isFloatType()) {
	if (from->isInteger() || from->isFloatType()) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isInteger()) {
	if (from->isInteger() || from->isFloatType()) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isPointer() && (from->isPointer() || from->isArray())) {
	assert(!to->isNullptr());
	if (from->isNullptr()) {
	    return to;
	}
	auto toRefTy = to->refType();
	auto fromRefTy = from->refType();
	if (!fromRefTy->hasConstFlag() && toRefTy->hasConstFlag()) {
	    toRefTy = toRefTy->getConstRemoved();
	}
	if (from->isArray() && equals(toRefTy, from)) {
	    return to;
	} else if (equals(toRefTy, fromRefTy)) {
	    return to;
	} else if (toRefTy->isVoid() || fromRefTy->isVoid()) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (from->isStruct() && to->isStruct()) {
	if (from->hasConstFlag() && !to->hasConstFlag()) {
	    from = from->getConstRemoved();
	}
	if (equals(from, to)) {
	    return to;
	} else {
	    return nullptr;
	}
    } else if (to->isArray() && from->isArray()) {
	if (to->dim() != from->dim()) {
	    return nullptr;
	}
	auto toRefTy = to->refType();
	auto fromRefTy = from->refType();
	if (fromRefTy->hasConstFlag() && !toRefTy->hasConstFlag()) {
	    fromRefTy = fromRefTy->getConstRemoved();
	}
	if (equals(toRefTy, fromRefTy)) {
	    return to;
	} else {
	    return nullptr;
	}
    } else {
	return nullptr;
    }
}

const Type *
Type::explicitCast(const Type *from, const Type *to)
{
    if (auto type = convert(from->getConstRemoved(), to->getConstRemoved())) {
	// allow const-casts
	return type;
    } else if (from->isPointer() && to->isPointer()) {
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

bool
Type::isScalar() const
{
    return !isArray() && !isStruct();
}

std::size_t
Type::aggregateSize() const
{
    if (isScalar()) {
	return 1;
    } else if (isArray()) {
	return dim();
    } else if (isStruct()) {
	return memberType().size();
    } else {
	assert(0);
    }
}

const Type *
Type::aggregateType(std::size_t index) const
{
    assert(index < aggregateSize());

    if (isScalar()) {
	return this;
    } else if (isArray()) {
	return refType();
    } else if (isStruct()) {
	return memberType()[index];
    } else {
	assert(0);
    }
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

// for floating point type (sub-)types 
bool
Type::isFloatType() const
{
    return isAlias() ? getUnalias()->isFloatType() : false;
}

bool
Type::isFloat() const
{
    return isAlias() ? getUnalias()->isFloat() : false;
}

bool
Type::isDouble() const
{
    return isAlias() ? getUnalias()->isDouble() : false;
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
