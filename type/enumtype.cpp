#include <cassert>
#include <memory>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "enumtype.hpp"

namespace abc {

static std::unordered_map<std::size_t, EnumType> enumMap;
static std::unordered_map<std::size_t, EnumType> enumConstMap;
static std::unordered_map<std::size_t, EnumType> enumVolatileMap;
static std::unordered_map<std::size_t, EnumType> enumConstVolatileMap;

//------------------------------------------------------------------------------

EnumType::EnumType(std::size_t id, UStr name, const Type *intType,
		   bool constFlag, bool volatileFlag)
    : Type{constFlag, volatileFlag, name}, id_{id}, intType{intType}
    , isComplete_{false}
{
}

void 
EnumType::init()
{
    enumMap.clear();
    enumConstMap.clear();
    enumVolatileMap.clear();
    enumConstVolatileMap.clear();
}

Type *
EnumType::createIncomplete(UStr name, const Type *intType)
{
    static std::size_t count;
    auto id = count++;

    enumMap.emplace(id, EnumType{id, name, intType, false, false});
    enumConstMap.emplace(id, EnumType{id, name, intType, true, false});
    enumVolatileMap.emplace(id, EnumType{id, name, intType, false, true});
    enumConstVolatileMap.emplace(id, EnumType{id, name, intType, true, true});

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
    if (hasVolatileFlag()) {
	return &enumConstVolatileMap.at(id_);
    } else {
	return &enumConstMap.at(id_);
    }
}

const Type *
EnumType::getVolatile() const
{
    if (hasConstFlag()) {
	return &enumConstVolatileMap.at(id_);
    } else {
	return &enumVolatileMap.at(id_);
    }
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
EnumType::complete(std::vector<UStr> &&constName,
		   std::vector<std::int64_t> &&constValue)
{
    constName_ = std::move(constName);
    constValue_ = std::move(constValue);
    enumMap.at(id_).isComplete_ = true;
    enumConstMap.at(id_).isComplete_ = true;
    enumVolatileMap.at(id_).isComplete_ = true;
    enumConstVolatileMap.at(id_).isComplete_ = true;
    return this;
}

const std::vector<UStr> &
EnumType::constName() const
{
    return constName_;
}

const std::vector<std::int64_t> &
EnumType::constValue() const
{
    return constValue_;
}

} // namespace abc
