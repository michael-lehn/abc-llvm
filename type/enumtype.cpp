#include <cassert>
#include <memory>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "enumtype.hpp"

namespace abc {

static std::unordered_map<std::size_t, EnumType> enumMap;
static std::unordered_map<std::size_t, EnumType> enumConstMap;

//------------------------------------------------------------------------------

EnumType::EnumType(std::size_t id, UStr name, const Type *intType,
		   bool constFlag)
    : Type{constFlag, name}, id_{id}, intType{intType}, isComplete_{false}
{
}

void 
EnumType::init()
{
    enumMap.clear();
    enumConstMap.clear();
}

Type *
EnumType::createIncomplete(UStr name, const Type *intType)
{
    static std::size_t count;
    auto id = count++;

    enumMap.emplace(id, EnumType{id, name, intType, false});
    enumConstMap.emplace(id, EnumType{id, name, intType, true});

    return &enumMap.at(id);
}

std::size_t
EnumType::id() const
{
    return id_;
}

const Type *
EnumType::getConst() const
{
    return &enumConstMap.at(id_);
}

const Type *
EnumType::getConstRemoved() const
{
    return &enumMap.at(id_);
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
EnumType::complete(std::vector<UStr> &&constName_,
		   std::vector<std::int64_t> &&constValue_)
{
    constName = std::move(constName_);
    constValue = std::move(constValue_);
    isComplete_ = true;
    return this;
}

} // namespace abc
