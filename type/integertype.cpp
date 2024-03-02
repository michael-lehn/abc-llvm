#include <memory>
#include <set>
#include <sstream>
#include <tuple>

#include "integertype.hpp"

namespace abc {

static bool
operator<(const IntegerType &x, const IntegerType &y)
{
    const auto &tx = std::tuple{x.numBits(),
				x.ustr().c_str(),
				x.aka().c_str(),
				x.isSignedInteger(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.numBits(),
				y.ustr().c_str(),
				y.aka().c_str(),
				y.isSignedInteger(),
				y.hasConstFlag()};

    return tx < ty;
}

static std::set<IntegerType> intSet;

//------------------------------------------------------------------------------

IntegerType::IntegerType(std::size_t numBits, bool signed_, bool constFlag,
			 UStr name)
    : Type{constFlag, name}, numBits_{numBits}, isSigned{signed_}
{
    std::stringstream ss;
    ss << (isSignedInteger() ? "i" : "u") << this->numBits();
    aka_ = UStr::create(ss.str());
}

const Type *
IntegerType::create(std::size_t numBits, bool signed_, bool constFlag,
		    UStr alias)
{
    auto ty = IntegerType{numBits, signed_, constFlag, alias};
    return &*intSet.insert(ty).first;
}

const Type *
IntegerType::createBool()
{
    return createUnsigned(1);
}

const Type *
IntegerType::createChar()
{
    return createUnsigned(8 * sizeof(char));
}

const Type *
IntegerType::createInt()
{
    return createSigned(8 * sizeof(int));
}


const Type *
IntegerType::createSigned(std::size_t numBits)
{
    return create(numBits, true, false, UStr{});
}

const Type *
IntegerType::createUnsigned(std::size_t numBits)
{
    return create(numBits, false, false, UStr{});
}

const Type *
IntegerType::getAlias(UStr alias) const
{
    return create(numBits(), isSignedInteger(), hasConstFlag(), alias);
}

const Type *
IntegerType::getConst() const
{
    return create(numBits(), isSignedInteger(), true, name);
}

const Type *
IntegerType::getConstRemoved() const
{
    return create(numBits(), isSignedInteger(), false, name);
}

bool
IntegerType::hasSize() const
{
    return true;
}

std::size_t
IntegerType::numBits() const
{
    return numBits_;
}

bool
IntegerType::isBool() const
{
    return numBits() == 1;
}

bool
IntegerType::isInteger() const
{
    return true;
}

bool
IntegerType::isSignedInteger() const
{
    return isSigned;
}

bool
IntegerType::isUnsignedInteger() const
{
    return !isSigned;
}

} // namespace abc
