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
				x.hasConstFlag(),
				x.hasVolatileFlag()};
    const auto &ty = std::tuple{y.floatKind,
				y.hasConstFlag(),
				y.hasVolatileFlag()};
    return tx < ty;
}

static std::set<FloatType> fltSet;

//------------------------------------------------------------------------------

FloatType::FloatType(FloatKind floatKind, bool constFlag, bool volatileFlag,
		     UStr name)
    : Type{constFlag, volatileFlag, name}, floatKind{floatKind}
{
}

const Type *
FloatType::create(FloatKind floatKind, bool constFlag, bool volatileFlag)
{
    std::string str = floatKind == FLOAT_KIND
	? "float"
	: "double";
    auto ty = FloatType{floatKind, constFlag, volatileFlag, UStr::create(str)};
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
    return create(FLOAT_KIND, false, false); 
}

const Type *
FloatType::createDouble()
{
    return create(DOUBLE_KIND, false, false); 
}

const Type *
FloatType::getConst() const
{
    return create(floatKind, true, hasVolatileFlag());
}

const Type *
FloatType::getVolatile() const
{
    return create(floatKind, hasConstFlag(), true);
}

const Type *
FloatType::getConstRemoved() const
{
    return create(floatKind, false, false);
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
