#include <set>

#include "autotype.hpp"

namespace abc {

bool
operator<(const AutoType &x, const AutoType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(), x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(), y.hasConstFlag()};

    return tx < ty;
}

static std::set<AutoType> voidSet;

//------------------------------------------------------------------------------

AutoType::AutoType(bool constFlag, UStr name) : Type{constFlag, name} {}

const Type *
AutoType::create(bool constFlag, UStr name)
{
    auto ty = AutoType{constFlag, name};
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
    return create(false, UStr::create("auto"));
}

const Type *
AutoType::getConst() const
{
    return create(true, name);
}

const Type *
AutoType::getConstRemoved() const
{
    return create(false, name);
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
