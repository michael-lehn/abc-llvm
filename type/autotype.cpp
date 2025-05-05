#include "autotype.hpp"

namespace abc {

static bool
operator<(const AutoType &x, const AutoType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.hasConstFlag(),
				x.hasVolatileFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.hasConstFlag(),
				y.hasVolatileFlag()};

    return tx < ty;
}

static std::set<AutoType> voidSet;

//------------------------------------------------------------------------------

AutoType::AutoType(bool constFlag, bool volatileFlag, UStr name)
    : Type{constFlag, volatileFlag, name}
{
}

const Type *
AutoType::create(bool constFlag, bool volatileFlag, UStr name)
{
    auto ty = AutoType{constFlag, volatileFlag, name};
    return &*voidSet.insert(ty).first;
}

void 
AutoType::init()
{
    voidSet.clear();
}

const Type *
AutoType::create()
{
    return create(false, false, UStr::create("auto"));
}

const Type *
AutoType::getConst() const
{
    return create(true, hasVolatileFlag(), name);
}

const Type *
AutoType::getVolatile() const
{
    return create(hasConstFlag(), true, name);
}

const Type *
AutoType::getConstRemoved() const
{
    return create(false, false, name);
}

bool
AutoType::hasSize() const
{
    return false;
}

bool
AutoType::isAuto() const
{
    return true;
}

} // namespace abc
