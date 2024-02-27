#include "type.hpp"

namespace type {

const Type *
Type::getAlias(const char *alias) const
{
    return getAlias(UStr::create(alias));
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

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    const char *constFlag = type->hasConstFlag() ? "const " : "";

    if (type->aka().c_str()) {
	out << constFlag << type->aka();
	out << " (aka '" << constFlag << type->ustr() << "')";
    } else {
	out << constFlag << type->ustr();
    }
    return out;
}

} // namespace type
