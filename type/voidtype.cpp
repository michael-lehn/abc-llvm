#include "voidtype.hpp"

namespace abc {

static bool
operator<(const VoidType &x, const VoidType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.hasConstFlag()};

    return tx < ty;
}

static std::set<VoidType> voidSet;

//------------------------------------------------------------------------------

VoidType::VoidType(bool constFlag, UStr name)
    : Type{constFlag, name}
{
}

const Type *
VoidType::create(bool constFlag, UStr name)
{
    auto ty = VoidType{constFlag, name};
    return &*voidSet.insert(ty).first;
}

void 
VoidType::init()
{
    voidSet.clear();
}

const Type *
VoidType::create()
{
    return create(false, UStr::create("void"));
}

const Type *
VoidType::getConst() const
{
    return create(true, name);
}

const Type *
VoidType::getConstRemoved() const
{
    return create(false, name);
}

bool
VoidType::hasSize() const
{
    return false;
}

bool
VoidType::isVoid() const
{
    return true;
}

} // namespace abc
