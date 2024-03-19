#include "typealias.hpp"

namespace abc {

static std::unordered_map<std::size_t, TypeAlias> aliasSet;
static std::unordered_map<std::size_t, TypeAlias> aliasConstSet;

//------------------------------------------------------------------------------
//
TypeAlias::TypeAlias(std::size_t id, UStr name, const Type *type,
		     bool constFlag)
    : Type{constFlag, name}, id{id}, type{type} 
{
}

void 
TypeAlias::init()
{
    aliasSet.clear();
    aliasConstSet.clear();
}

const Type *
TypeAlias::create(UStr name, const Type *type)
{
    static std::size_t count;
    auto id = count++;

    aliasSet.emplace(id, TypeAlias{id, name, type, false});
    aliasConstSet.emplace(id, TypeAlias{id, name, type, true});

    return &aliasSet.at(id);
}

bool
TypeAlias::isAlias() const
{
    return true;
}

const Type *
TypeAlias::getUnalias() const
{
    return type;
}

const Type *
TypeAlias::getConst() const
{
    return &aliasConstSet.at(id);
}

const Type *
TypeAlias::getConstRemoved() const
{
    return &aliasSet.at(id);
}

} // namespace abc
