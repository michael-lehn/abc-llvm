#include <cassert>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "error.hpp"
#include "gen.hpp"
#include "symtab.hpp"

namespace symtab {

//static std::unordered_map<UStr, SymEntry> symtab;

struct ScopeNode
{
    std::unordered_map<const char *, SymEntry> symtab;
    std::unique_ptr<ScopeNode> up;
    std::size_t id = 0;

    ScopeNode() { }
    ~ScopeNode() { }
};

static std::unique_ptr<ScopeNode> curr = std::make_unique<ScopeNode>();
static ScopeNode *root = curr.get();

void
openScope(void)
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
closeScope(void)
{
    assert(curr->up);

    curr = std::move(curr->up);
}

static SymEntry *
get(UStr ident, ScopeNode *begin, ScopeNode *end)
{
    for (ScopeNode *sn = begin; sn != end; sn = sn->up.get()) {
	if (sn->symtab.contains(ident.c_str())) {
	    return &sn->symtab[ident.c_str()];
	}
    }
    return nullptr;
}

SymEntry *
get(UStr ident, Scope scope)
{
    ScopeNode *begin = scope == RootScope ? root : curr.get();
    ScopeNode *end = scope == AnyScope ? nullptr : begin->up.get();
    return get(ident, begin, end);
}

static SymEntry *
add(Token::Loc loc, UStr ident, const Type *type, ScopeNode *sn)
{
    assert(sn);
    SymEntry *found = get(ident, sn, sn->up.get());

    if (found) {
	// already in symtab
	error::out() << loc << ": identifier '" << ident.c_str()
	    << "' already defined. Previous definition at " << found->loc
	    << std::endl;
	error::fatal();
	return found;
    }

    UStr internalIdent = ident;
    if (sn->id) {
	std::stringstream ss;
	ss << "._" << ident.c_str() << sn->id << "_.";
	internalIdent = ss.str();
    }

    sn->symtab[ident.c_str()] = {loc, type, ident, internalIdent};
    return &sn->symtab[ident.c_str()];
}


SymEntry *
add(Token::Loc loc, UStr ident, const Type *type)
{
    return add(loc, ident, type, curr.get());
}

SymEntry *
addToRootScope(Token::Loc loc, UStr ident, const Type *type)
{
    return add(loc, ident, type, root);
}

UStr
addStringLiteral(UStr str)
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
	addToRootScope(Token::Loc{}, ident, ty);
	gen::defStringLiteral(ident.c_str(), str.c_str(), true);
    }
    return ident;

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
	    << sym.second.loc << ", "
	    << sym.second.type << ", "
	    << sym.second.internalIdent.c_str()
	    << std::endl;
    }
    out << std::setfill(' ') << std::setw(indent * 4) << " " << "end of scope"
	<< indent << std::endl;

    return indent + 1;
}

std::ostream &
print(std::ostream &out)
{
    out << " --- symtab ----" << std::endl;
    print(out, curr.get());
    out << " ---------------" << std::endl;
    return out;
}

} // namespace symtab
