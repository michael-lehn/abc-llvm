#include <set>
#include <sstream>

#include "pointertype.hpp"

namespace abc {

static bool
operator<(const PointerType &x, const PointerType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.refType(),
				x.hasConstFlag(),
				x.hasVolatileFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.refType(),
				y.hasConstFlag(),
				y.hasVolatileFlag()};
    return tx < ty;
}

static std::set<PointerType> pointerSet;

//------------------------------------------------------------------------------

PointerType::PointerType(const Type *refType, bool constFlag,
			 bool volatileFlag, UStr name)
    : Type{constFlag, volatileFlag, name}, refType_{refType}
{
}

const Type *
PointerType::create(const Type *refType, bool constFlag, bool volatileFlag)
{
    std::stringstream ss;
    ss << "-> " << refType;
    auto ty = PointerType{refType, constFlag, volatileFlag,
			  UStr::create(ss.str())};
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
    return create(refType, false, false);
}

const Type *
PointerType::getVolatile() const
{
    return create(refType(), hasConstFlag(), true);
}

const Type *
PointerType::getConst() const
{
    return create(refType(), true, hasVolatileFlag());
}

const Type *
PointerType::getConstRemoved() const
{
    return create(refType(), false, false);
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
