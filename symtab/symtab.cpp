#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "gen/editdistance.hpp"
#include "lexer/error.hpp"

#include "symtab.hpp"

namespace abc {

std::forward_list<std::unique_ptr<Symtab::ScopeNode>> Symtab::scope;
std::size_t Symtab::scopeSize;
UStr Symtab::scopePrefix;
std::size_t idCount;

Symtab::Symtab(UStr scopePrefix_)
{
    // prefix required one below root scope (identify function)
    assert((scopeSize == 1 && scopePrefix_.c_str()) ||
           (scopeSize != 1 && !scopePrefix_.c_str()));

    if (scopePrefix_.c_str() || scopeSize == 0) {
	scopePrefix = scopePrefix_;
	idCount = 0;
    }
    scope.push_front(std::make_unique<ScopeNode>());
    ++scopeSize;
}

Symtab::~Symtab()
{
    scope.pop_front();
    --scopeSize;
}

std::vector<std::string>
Symtab::didYouMean(UStr name_)
{
    std::vector<std::string> list;
    std::string name{name_.c_str()};

    unsigned max = 2;
    for (auto s = scope.cbegin(); s != scope.cend(); ++s, max = 1) {
	for (const auto &node : **s) {
	    if (!node.second.typeDeclaration()) {
		std::string id{node.first.c_str()};
		if (gen::editDistance(id, name) <= max) {
		    list.push_back(id);
		}
	    }
	}
    }
    return list;
}

const symtab::Entry *
Symtab::find(UStr name, Scope inScope)
{
    for (auto s = scope.cbegin(); s != scope.cend(); ++s) {
	for (const auto &node : **s) {
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

const symtab::Entry *
Symtab::type(UStr name, Scope inScope)
{
    auto entry = find(name, inScope);
    if (entry && entry->typeDeclaration()) {
	return entry;
    }
    return nullptr;
}

const symtab::Entry *
Symtab::variable(UStr name, Scope inScope)
{
    auto entry = find(name, inScope);
    if (entry && entry->variableDeclaration()) {
	return entry;
    }
    return nullptr;
}

const symtab::Entry *
Symtab::constant(UStr name, Scope inScope)
{
    auto entry = find(name, inScope);
    if (entry && entry->expressionDeclaration()) {
	return entry;
    }
    return nullptr;
}

std::pair<symtab::Entry *, bool>
Symtab::addDeclaration(lexer::Loc loc, UStr name, const Type *type)
{
    auto id = getId(name);
    return add(name, symtab::Entry::createVarEntry(loc, id, type));
}

std::pair<symtab::Entry *, bool>
Symtab::addDefinition(lexer::Loc loc, UStr name, const Type *type)
{
    auto decl = addDeclaration(loc, name, type);
    if (!decl.first->setDefinitionFlag()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD) << name
	             << " already defined in this scope.\n"
	             << error::setColor(error::NORMAL);
	assert(decl.first);
	error::location(decl.first->loc);
	error::out() << error::setColor(error::BOLD) << decl.first->loc
	             << ": defined here.\n"
	             << error::setColor(error::NORMAL);
	error::fatal();

	return {nullptr, false};
    }
    return decl;
}

std::pair<symtab::Entry *, bool>
Symtab::addType(lexer::Loc loc, UStr name, const Type *type)
{
    auto id = getId(name);
    return add(name, symtab::Entry::createTypeEntry(loc, id, type));
}

std::pair<symtab::Entry *, bool>
Symtab::addExpression(lexer::Loc loc, UStr name, const Expr *expr)
{
    auto id = getId(name);
    return add(name, symtab::Entry::createExprEntry(loc, id, expr));
}

void
Symtab::print(std::ostream &out)
{
    out << "Symtab (from current scope to root scope):\n";
    std::for_each(scope.begin(), scope.end(), [&](const auto &s) {
	for (const auto &item : *s) {
	    out << item.first << ": " << item.second.getId() << ", ";
	    if (item.second.expressionDeclaration()) {
		out << item.second.expr;
	    } else {
		out << item.second.type;
	    }
	    out << "\n";
	}
	out << "---\n";
    });
    out << "End of symtab\n";
}

std::pair<symtab::Entry *, bool>
Symtab::add(UStr name, symtab::Entry &&entry)
{
    if (scope.front()->contains(name)) {
	auto &found = scope.front()->at(name);

	bool changed = false;
	if (entry != found) {
	    bool ok = false;

	    // check exceptions ...
	    if (entry.variableDeclaration() && found.variableDeclaration()) {
		bool fixUnbound =
		    found.type->isUnboundArray() && entry.type->isArray() &&
		    found.type->refType() == entry.type->refType();
		bool resolveAuto =
		    found.type->isAuto() && entry.type->hasSize();

		ok = fixUnbound || resolveAuto;
		if (ok) {
		    found.type = entry.type;
		    changed = true;
		}
	    }

	    if (!ok) {
		error::location(entry.loc);
		error::out() << error::setColor(error::BOLD) << entry.loc
		             << ": " << error::setColor(error::BOLD_RED)
		             << "error: " << error::setColor(error::BOLD)
		             << "incompatible redefinition with type '"
		             << entry.type << "'\n"
		             << error::setColor(error::NORMAL);
		error::location(found.loc);
		error::out()
		    << error::setColor(error::BOLD) << found.loc << ": "
		    << error::setColor(error::BOLD_RED)
		    << "error: " << error::setColor(error::BOLD)
		    << "previous definition with type '" << found.type << "'\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
		return {nullptr, false};
	    }
	}
	return {&found, changed};
    }

    auto added = scope.front()->insert({name, std::move(entry)});
    assert(added.second);
    return {&(*added.first).second, true};
}

UStr
Symtab::getId(UStr name)
{
    assert(!name.empty());
    std::string idStr = name.c_str();
    if (scopeSize > 1) {
	assert(scopePrefix.c_str());
	std::stringstream ss;
	ss << "." << scopePrefix.c_str() << "." << idStr << "." << idCount++;
	idStr = ss.str();
    }
    return UStr::create(idStr);
}

} // namespace abc
