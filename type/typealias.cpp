#include <iostream>

#include "typealias.hpp"

namespace abc {

static std::unordered_map<std::size_t, TypeAlias> aliasSet;
static std::unordered_map<std::size_t, TypeAlias> aliasConstSet;
static std::unordered_map<std::size_t, TypeAlias> aliasVolatileSet;
static std::unordered_map<std::size_t, TypeAlias> aliasConstVolatileSet;

//------------------------------------------------------------------------------
//
TypeAlias::TypeAlias(std::size_t id, UStr name, const Type *type,
		     bool constFlag, bool volatileFlag)
    : Type{constFlag, volatileFlag, name}, id{id}, type{type} 
{
}

void 
TypeAlias::init()
{
    aliasSet.clear();
    aliasConstSet.clear();
    aliasVolatileSet.clear();
    aliasConstVolatileSet.clear();
}

const Type *
TypeAlias::create(UStr name, const Type *type)
{
    static std::size_t count;
    auto id = count++;

    aliasSet.emplace(id, TypeAlias{id, name, type, false, false});
    aliasConstSet.emplace(id, TypeAlias{id, name, type, true, false});
    aliasVolatileSet.emplace(id, TypeAlias{id, name, type, false, true});
    aliasConstVolatileSet.emplace(id, TypeAlias{id, name, type, true, true});

    return  &aliasSet.at(id);
}

bool
TypeAlias::isAlias() const
{
    return true;
}

const Type *
TypeAlias::getUnalias() const
{
    auto ty = type;
    if (isConst) {
	ty = ty->getConst();
    }
    if (isVolatile) {
	ty = ty->getVolatile();
    }
    return ty;
}

const Type *
TypeAlias::getConst() const
{
    if (hasVolatileFlag()) {
	return &aliasConstVolatileSet.at(id);
    } else {
	return &aliasConstSet.at(id);
    }
}

const Type *
TypeAlias::getVolatile() const
{
    if (hasConstFlag()) {
	return &aliasConstVolatileSet.at(id);
    } else {
	return &aliasVolatileSet.at(id);
    }
}

const Type *
TypeAlias::getConstRemoved() const
{
    return &aliasSet.at(id);
}

} // namespace abc
