#include <cassert>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "error.hpp"
#include "gen.hpp"
#include "symtab.hpp"

struct ScopeNode
{
    std::unordered_map<const char *, Symtab::Entry> symtab;
    std::unordered_map<const char *, const Type *> typetab;
    std::unique_ptr<ScopeNode> up;
    std::size_t id = 0;

    ScopeNode() { }
    ~ScopeNode() { }
};

static std::unique_ptr<ScopeNode> curr = std::make_unique<ScopeNode>();
static ScopeNode *root = curr.get();

//------------------------------------------------------------------------------

bool
Symtab::Entry::setDefinitionFlag(void)
{
    if (definition_) {
	error::out() << lastDeclLoc << ": '" << ident.c_str()
	    << "' already defined. Previous definition at "
	    << loc << std::endl;
	error::fatal();
	return false; // failed
    }
    definition_ = true;
    loc = lastDeclLoc;
    return true; // success
}

//------------------------------------------------------------------------------

void
Symtab::openScope(void)
{
    static std::size_t id;

    if (curr.get() == root) {
	id = 0;
    }

    std::unique_ptr<ScopeNode> s = std::make_unique<ScopeNode>();
    s->up = std::move(curr);
    s->id = ++id;
    curr = std::move(s);

}

void
Symtab::closeScope(void)
{
    assert(curr->up);

    curr = std::move(curr->up);
}

Symtab::Entry *
Symtab::get(UStr ident, ScopeNode *begin, ScopeNode *end)
{
    for (ScopeNode *sn = begin; sn != end; sn = sn->up.get()) {
	if (sn->symtab.contains(ident.c_str())) {
	    return &sn->symtab.at(ident.c_str());
	}
    }
    return nullptr;
}

Symtab::Entry *
Symtab::get(UStr ident, Scope scope)
{
    ScopeNode *begin = scope == RootScope ? root : curr.get();
    ScopeNode *end = scope == AnyScope ? nullptr : begin->up.get();
    return get(ident, begin, end);
}

Symtab::Entry *
Symtab::add(Token::Loc loc, UStr ident, const Type *type, ScopeNode *sn)
{
    assert(sn);
    Symtab::Entry *found = get(ident, sn, sn->up.get());

    if (found) {
	found->lastDeclLoc = loc;
	return found;
    }

    UStr internalIdent = ident;
    if (sn->id) {
	std::stringstream ss;
	ss << "._" << ident.c_str() << sn->id << "_.";
	internalIdent = ss.str();
    }

    auto entry = Entry{loc, type, ident, internalIdent};
    sn->symtab.emplace(ident.c_str(), std::move(entry));
    return &sn->symtab.at(ident.c_str());
}

Symtab::Entry *
Symtab::addDecl(Token::Loc loc, UStr ident, const Type *type)
{
    return add(loc, ident, type, curr.get());
}

Symtab::Entry *
Symtab::addDeclToRootScope(Token::Loc loc, UStr ident, const Type *type)
{
    return add(loc, ident, type, root);
}

UStr
Symtab::addStringLiteral(UStr str)
{
    static std::size_t id;
    static std::unordered_map<const char *, std::size_t> pool;

    bool added = false;
    if (!pool.contains(str.c_str())) {
	pool[str.c_str()] = id++;
	added = true;
    }
    std::stringstream ss;
    ss << ".str" << pool[str.c_str()];
    UStr ident{ss.str()};

    if (added) {
	auto ty = Type::getArray(Type::getUnsignedInteger(8),
				 strlen(str.c_str()) + 1);
	addDeclToRootScope(Token::Loc{}, ident, ty);
	gen::defStringLiteral(ident.c_str(), str.c_str(), true);
    }
    return ident;

}

const Type *
Symtab::getNamedType(UStr ident, ScopeNode *begin, ScopeNode *end)
{
    for (ScopeNode *sn = begin; sn != end; sn = sn->up.get()) {
	if (sn->typetab.contains(ident.c_str())) {
	    return sn->typetab.at(ident.c_str());
	}
    }
    return nullptr;
}

const Type *
Symtab::getNamedType(UStr ident, Scope scope)
{
    ScopeNode *begin = scope == RootScope ? root : curr.get();
    ScopeNode *end = scope == AnyScope ? nullptr : begin->up.get();
    return Symtab::getNamedType(ident, begin, end);
}

const Type *
Symtab::getNamedType(UStr ident)
{
    return getNamedType(ident, AnyScope);
}

const Type *
Symtab::createTypeAlias(UStr ident, const Type *type)
{
    assert(curr.get());
    auto *found = getNamedType(ident, CurrentScope);

    if (found) {
	return nullptr;
    }

    curr.get()->typetab.emplace(ident.c_str(), type);
    return type;
}

static std::size_t
print(std::ostream &out, ScopeNode *sn)
{
    if (!sn) {
	return 0;
    }

    std::size_t indent = print(out, sn->up.get());
    out << std::setfill(' ') << std::setw(indent * 4) << " " << "start of scope"
	<< indent << std::endl;
    for (auto sym: sn->symtab) {
	out << std::setfill(' ') << std::setw(indent * 4) << " "
	    << sym.second.ident.c_str() << ": "
	    << sym.second.getLoc() << ", "
	    << sym.second.getType() << ", "
	    << sym.second.internalIdent().c_str()
	    << std::endl;
    }
    out << std::setfill(' ') << std::setw(indent * 4) << " " << "end of scope"
	<< indent << std::endl;

    return indent + 1;
}

std::ostream &
Symtab::print(std::ostream &out)
{
    out << " --- symtab ----" << std::endl;
    ::print(out, curr.get());
    out << " ---------------" << std::endl;
    return out;
}
