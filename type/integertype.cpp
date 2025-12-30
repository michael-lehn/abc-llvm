#include <set>
#include <sstream>
#include <tuple>

#include "integertype.hpp"

namespace abc {

bool
operator<(const IntegerType &x, const IntegerType &y)
{
    const auto &tx = std::tuple{x.numBits(), x.ustr().c_str(),
                                x.isSignedInteger(), x.hasConstFlag()};
    const auto &ty = std::tuple{y.numBits(), y.ustr().c_str(),
                                y.isSignedInteger(), y.hasConstFlag()};

    return tx < ty;
}

static std::set<IntegerType> intSet;

//------------------------------------------------------------------------------

IntegerType::IntegerType(std::size_t numBits, bool signed_, bool constFlag,
                         UStr name)
    : Type{constFlag, name}, numBits_{numBits}, isSigned{signed_}
{
}

const Type *
IntegerType::create(std::size_t numBits, bool signed_, bool constFlag)
{
    std::stringstream ss;
    ss << (signed_ ? "i" : "u") << numBits;
    auto ty = IntegerType{numBits, signed_, constFlag, UStr::create(ss.str())};
    return &*intSet.insert(ty).first;
}

void
IntegerType::init()
{
    intSet.clear();
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
IntegerType::createUnsigned()
{
    return createUnsigned(8 * sizeof(unsigned));
}

const Type *
IntegerType::createLong()
{
    return createSigned(8 * sizeof(long));
}

const Type *
IntegerType::createSizeType()
{
    return createSigned(8 * sizeof(std::size_t));
}

const Type *
IntegerType::createPtrdiffType()
{
    return createSigned(8 * sizeof(std::ptrdiff_t));
}

const Type *
IntegerType::createSigned(std::size_t numBits)
{
    return create(numBits, true, false);
}

const Type *
IntegerType::createUnsigned(std::size_t numBits)
{
    return create(numBits, false, false);
}

const Type *
IntegerType::getConst() const
{
    return create(numBits(), isSignedInteger(), true);
}

const Type *
IntegerType::getConstRemoved() const
{
    return create(numBits(), isSignedInteger(), false);
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
