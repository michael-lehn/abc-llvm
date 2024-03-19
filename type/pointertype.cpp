#include <set>
#include <sstream>

#include "pointertype.hpp"

namespace abc {

static bool
operator<(const PointerType &x, const PointerType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.refType(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.refType(),
				y.hasConstFlag()};
    return tx < ty;
}

static std::set<PointerType> pointerSet;

//------------------------------------------------------------------------------

PointerType::PointerType(const Type *refType, bool constFlag, UStr name)
    : Type{constFlag, name}, refType_{refType}
{
}

const Type *
PointerType::create(const Type *refType, bool constFlag)
{
    std::stringstream ss;
    ss << "-> " << refType;
    auto ty = PointerType{refType, constFlag, UStr::create(ss.str())};
    return &*pointerSet.insert(ty).first;
}

void 
PointerType::init()
{
    pointerSet.clear();
}

const Type *
PointerType::create(const Type *refType)
{
    return create(refType, false);
}

const Type *
PointerType::getConst() const
{
    return create(refType(), true);
}

const Type *
PointerType::getConstRemoved() const
{
    return create(refType(), false);
}

bool
PointerType::isPointer() const
{
    return true;
}

const Type *
PointerType::refType() const
{
    return refType_;
}

} // namespace abc
