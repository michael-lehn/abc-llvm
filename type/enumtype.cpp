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

EnumType::EnumType(std::size_t id, UStr name, const Type *intType,
		   bool constFlag)
    : Type{constFlag, name}, id{id}, intType{intType}, isComplete_{false}
{
}

Type *
EnumType::createIncomplete(UStr name, const Type *intType)
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
EnumType::hasSize() const
{
    return isComplete_;
}

std::size_t
EnumType::numBits() const
{
    return intType->numBits();
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

bool
EnumType::isEnum() const
{
    return true;
}

const Type *
EnumType::complete(const std::vector<UStr> &&constName_,
		   const std::vector<std::int64_t> &&constValue_)
{
    constName = std::move(constName_);
    constValue = std::move(constValue_);
    isComplete_ = true;
    return this;
}

} // namespace abc
