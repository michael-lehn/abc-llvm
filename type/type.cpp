#include "type.hpp"

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
    } else {
	return nullptr;
    }
}

const Type *
Type::getAlias(const char *alias) const
{
    return getAlias(UStr::create(alias));
}

UStr
Type::ustr() const
{
    return name;
}

UStr
Type::aka() const
{
    return aka_;
}

bool
Type::hasConstFlag() const
{
    return isConst;
}

bool
Type::isVoid() const
{
    return false;
}

bool
Type::isBool() const
{
    return false;
}

bool
Type::isNullptr() const
{
    return false;
}

// for integer (sub-)types 
bool
Type::isInteger() const
{
    return false;
}

bool
Type::isSignedInteger() const
{
    return false;
}

bool
Type::isUnsignedInteger() const
{
    return false;
}

std::size_t
Type::numBits() const
{
    return 0;
}

// for pointer and array (sub-)types
bool
Type::isPointer() const
{
    return false;
}

bool
Type::isArray() const
{
    return false;
}

const Type *
Type::refType() const
{
    return nullptr;
}

std::size_t
Type::dim() const
{
    return 0;
}

// for function (sub-)types 
bool
Type::isFunction() const
{
    return false;
}

const Type *
Type::retType() const
{
    return nullptr;
}

bool
Type::hasVarg() const
{
    return false;
}

const
std::vector<const Type *> &
Type::paramType() const
{
    static std::vector<const Type *> noArgs;
    return noArgs;
}

// for struct (sub-)types
bool
Type::isStruct() const
{
    return false;
}

const Type *
Type::complete(const std::vector<UStr> &&,
	       const std::vector<const Type *> &&)
{
    return nullptr;
}

const std::vector<const Type *> &
Type::memberType() const
{
    static std::vector<const Type *> noMembers;
    return noMembers;
}

const std::vector<UStr> &
Type::memberIdent() const
{
    static std::vector<UStr> noMembers;
    return noMembers;
}

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    const char *constFlag = type->hasConstFlag() ? "const " : "";

    if (type->ustr().c_str()) {
	out << constFlag << type->ustr();
    } else {
	out << constFlag << type->aka();
    }
    return out;
}

} // namespace abc
