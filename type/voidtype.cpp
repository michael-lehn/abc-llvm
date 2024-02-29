#include "voidtype.hpp"

namespace abc {

static bool
operator<(const VoidType &x, const VoidType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.aka().c_str(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.aka().c_str(),
				y.hasConstFlag()};

    return tx < ty;
}

static std::set<VoidType> voidSet;

//------------------------------------------------------------------------------

VoidType::VoidType(bool constFlag, UStr alias)
    : Type{alias}, isConst{constFlag}
{
    name = UStr::create("void");
}

const Type *
VoidType::create(bool constFlag, UStr alias)
{
    auto ty = VoidType{constFlag, alias};
    return &*voidSet.insert(ty).first;
}

const Type *
VoidType::create()
{
    return create(false, UStr{});
}

const Type *
VoidType::getAlias(UStr alias) const
{
    return create(hasConstFlag(), alias);
}

const Type *
VoidType::getConst() const
{
    return create(true, alias);
}

const Type *
VoidType::getConstRemoved() const
{
    return create(false, alias);
}

bool
VoidType::hasSize() const
{
    return false;
}

bool
VoidType::hasConstFlag() const
{
    return isConst;
}

bool
VoidType::isVoid() const
{
    return true;
}

} // namespace abc
