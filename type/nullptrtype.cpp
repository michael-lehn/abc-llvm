#include <cassert>

#include "nullptrtype.hpp"

namespace abc {

static bool
operator<(const NullptrType &x, const NullptrType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.hasConstFlag()};

    return tx < ty;
}

static std::set<NullptrType> nullptrSet;

//------------------------------------------------------------------------------

NullptrType::NullptrType(bool constFlag, UStr name)
    : Type{constFlag, name}
{
}

const Type *
NullptrType::create(bool constFlag, UStr name)
{
    auto ty = NullptrType{constFlag, name};
    return &*nullptrSet.insert(ty).first;
}

const Type *
NullptrType::create()
{
    return create(false, UStr::create("nullptr"));
}

const Type *
NullptrType::getConst() const
{
    return create(true, name);
}

const Type *
NullptrType::getConstRemoved() const
{
    return create(false, name);
}

bool
NullptrType::hasSize() const
{
    return false;
}

bool
NullptrType::isNullptr() const
{
    return true;
}

bool
NullptrType::isPointer() const
{
    return true;
}

const Type *
NullptrType::refType() const
{
    assert(0 && "Nullptr has no referenced type");
    return nullptr;
}

} // namespace abc
