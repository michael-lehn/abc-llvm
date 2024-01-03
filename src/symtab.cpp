#include <cassert>
#include <iostream>
#include <iomanip>
#include <memory>
#include <unordered_map>

#include "symtab.hpp"

namespace symtab {

//static std::unordered_map<UStr, SymEntry> symtab;

struct ScopeNode
{
    std::unordered_map<UStr, SymEntry> symtab;
    std::unique_ptr<ScopeNode> up;

    ScopeNode() { std::cerr << "New scope." << std::endl; }
    ~ScopeNode() { std::cerr << "Closing scope." << std::endl; }
};

static std::unique_ptr<ScopeNode> curr = std::make_unique<ScopeNode>();
static ScopeNode *root = curr.get();

void
openScope(void)
{
    std::unique_ptr<ScopeNode> s = std::make_unique<ScopeNode>();
    s->up = std::move(curr);
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
	if (sn->symtab.contains(ident)) {
	    return &sn->symtab[ident];
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
	return nullptr;
    }

    sn->symtab[ident] = {loc, type, ident};
    return &sn->symtab[ident];
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
	    << sym.first.c_str() << ": "
	    << sym.second.loc << ", "
	    << sym.second.type << std::endl;
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
