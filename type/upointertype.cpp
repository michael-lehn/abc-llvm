#include <iostream>
#include <set>
#include <sstream>

#include "functiontype.hpp"
#include "pointertype.hpp"
#include "upointertype.hpp"
#include "voidtype.hpp"

namespace abc {

static bool
operator<(const UPointerType &x, const UPointerType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.refType(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.refType(),
				y.hasConstFlag()};
    return tx < ty;
}

static std::set<UPointerType> pointerSet;

//------------------------------------------------------------------------------

UPointerType::UPointerType(const Type *refType, UStr destructorName,
			   bool constFlag, UStr name)
    : Type{constFlag, name}, refType_{refType}, destructorName_{destructorName}
{
}

const Type *
UPointerType::create(const Type *refType, UStr destructorName, bool constFlag)
{
    std::stringstream ss;
    ss << "=> " << refType;
    auto name = UStr::create(ss.str());
    auto ty = UPointerType{refType, destructorName, constFlag, name};
    return &*pointerSet.insert(ty).first;
}

void 
UPointerType::init()
{
    pointerSet.clear();
}

const Type *
UPointerType::create(const Type *refType, UStr destructorName)
{
    return create(refType, destructorName, false);
}

const Type *
UPointerType::destructorType(const Type *refType)
{
    auto argTy = PointerType::create(PointerType::create(refType));
    return FunctionType::create(VoidType::create(), {argTy});
}

const Type *
UPointerType::getConst() const
{
    return create(refType(), destructorName(), true);
}

const Type *
UPointerType::getConstRemoved() const
{
    return create(refType(), destructorName(), false);
}

bool
UPointerType::isPointer() const
{
    return true;
}

bool
UPointerType::isUPointer() const
{
    return true;
}

const Type *
UPointerType::refType() const
{
    return refType_;
}

const UStr
UPointerType::destructorName() const
{
    return destructorName_;
}


} // namespace abc
