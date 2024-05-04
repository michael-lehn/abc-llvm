#include <memory>
#include <set>
#include <sstream>
#include <tuple>

#include "floattype.hpp"

namespace abc {

bool
operator<(const FloatType &x, const FloatType &y)
{
    const auto &tx = std::tuple{x.floatKind,
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.floatKind,
				y.hasConstFlag()};
    return tx < ty;
}

static std::set<FloatType> fltSet;

//------------------------------------------------------------------------------

FloatType::FloatType(FloatKind floatKind, bool constFlag, UStr name)
    : Type{constFlag, name}, floatKind{floatKind}
{
}

const Type *
FloatType::create(FloatKind floatKind, bool constFlag)
{
    std::string str = floatKind == FLOAT_KIND
	? "float"
	: "double";
    auto ty = FloatType{floatKind, constFlag, UStr::create(str)};
    return &*fltSet.insert(ty).first;
}

void 
FloatType::init()
{
    fltSet.clear();
}

const Type *
FloatType::createFloat()
{
    return create(FLOAT_KIND, false); 
}

const Type *
FloatType::createDouble()
{
    return create(DOUBLE_KIND, false); 
}

const Type *
FloatType::getConst() const
{
    return create(floatKind, true);
}

const Type *
FloatType::getConstRemoved() const
{
    return create(floatKind, false);
}

bool
FloatType::isFloatType() const
{
    return true;
}

bool
FloatType::isFloat() const
{
    return floatKind == FLOAT_KIND;
}

bool
FloatType::isDouble() const
{
    return floatKind == DOUBLE_KIND;
}

} // namespace abc
