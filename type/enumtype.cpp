#include <cassert>
#include <memory>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "enumtype.hpp"

namespace abc {

static std::unordered_map<std::size_t, EnumType> enumSet;
static std::unordered_map<std::size_t, EnumType> enumConstSet;

//------------------------------------------------------------------------------

EnumType::EnumType(std::size_t id, UStr name, Type *intType, bool constFlag)
    : Type{constFlag, name}, id{id}, intType{intType}, isComplete_{false}
{
}

const Type *
EnumType::create(UStr name, Type *intType)
{
    static std::size_t count;
    auto id = count++;

    enumSet.emplace(id, EnumType{id, name, intType, false});
    enumConstSet.emplace(id, EnumType{id, name, intType, true});

    return &enumSet.at(id);
}

const Type *
EnumType::getConst() const
{
    return &enumConstSet.at(id);
}

const Type *
EnumType::getConstRemoved() const
{
    return &enumSet.at(id);
}

bool
EnumType::isComplete() const
{
    return isComplete_;
}

bool
EnumType::hasSize() const
{
    return isComplete();
}

std::size_t
EnumType::numBits() const
{
    if (isComplete()) {
	return intType->numBits();
    } else {
	return 0;
    }
}

bool
EnumType::isInteger() const
{
    return true;
}

bool
EnumType::isSignedInteger() const
{
    return intType->isSignedInteger();
}

bool
EnumType::isUnsignedInteger() const
{
    return intType->isUnsignedInteger();
}

} // namespace abc
