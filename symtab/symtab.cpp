#include <cassert>
#include <sstream>
#include <string>

#include "lexer/error.hpp"

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
    name = getId(name);
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

void
Symtab::print(std::ostream &out)
{
    out << "Symtab (from current scope to root scope):\n";
    std::for_each(scope.begin(), scope.end(),
	    [&](const auto &s) {
		for (const auto &item: *s) {
		    out << item.first << ": "
			<< item.second.id << ", "
			<< item.second.type
			<< "\n";
		}
		out << "---\n";
	    });
    out << "End of symtab\n";
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
