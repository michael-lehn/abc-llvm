#include <cassert>
#include <sstream>
#include <string>

#include "symtab.hpp"

namespace abc {

std::forward_list<std::unique_ptr<Symtab::ScopeNode>> Symtab::scope;
std::size_t Symtab::scopeSize;
UStr Symtab::scopePrefix;

Symtab::Symtab(UStr scopePrefix_)
{
    // prefix required one below root scope (identify function)
    assert((scopeSize == 1 && scopePrefix_.c_str())
	|| (scopeSize != 1 && !scopePrefix_.c_str()));

    scopePrefix = scopePrefix_;
    scope.push_front(std::make_unique<ScopeNode>());
    ++scopeSize;
}

Symtab::~Symtab()
{
    scope.pop_front();
    --scopeSize;
}

const symtab::Entry *
Symtab::find(UStr name, Scope inScope)
{
    for (auto s = scope.cbegin(); s != scope.cend(); ++s) {
	for (const auto &node: **s) {
	    if (node.first == name) {
		return &node.second;
	    }
	}
	if (inScope == CurrentScope) {
	    break;
	}
    }
    return nullptr;
}

std::pair<symtab::Entry *, bool>
Symtab::addDeclaration(lexer::Loc loc, UStr name, const Type *type)
{
    auto id = getId(name);
    if (scope.front()->contains(id)) {
	return {&scope.front()->at(id), false};
    }

    auto entry = symtab::Entry{loc, id, type};
    auto added = scope.front()->insert({id, entry});
    assert(added.second);
    return {&(*added.first).second, true};
}

UStr
Symtab::getId(UStr name)
{
    std::string idStr = name.c_str();
    if (scopeSize > 1) {
	std::stringstream ss;
	ss << "." << scopePrefix.c_str() << "." << idStr << "." << scopeSize;
	idStr = ss.str();
    }
    return UStr::create(idStr);
}

} // namespace abc
