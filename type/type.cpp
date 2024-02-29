#include "type.hpp"

namespace abc {

Type::Type(UStr alias)
    : alias{alias}
{
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
    return alias;
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
Type::argType() const
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

    if (type->aka().c_str()) {
	out << constFlag << type->aka();
    } else {
	out << constFlag << type->ustr();
    }
    return out;
}

} // namespace abc
