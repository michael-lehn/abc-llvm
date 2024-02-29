#include <cassert>

#include "entry.hpp"

namespace abc { namespace symtab {

Entry::Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type)
    : kind{kind}, loc{loc}, id{id}, type{type}
{
}

Entry
Entry::createVarEntry(lexer::Loc loc, UStr id, const Type *type)
{
    return Entry(VAR, loc, id, type);
}

Entry
Entry::createTypeEntry(lexer::Loc loc, UStr id, const Type *type)
{
    return Entry(TYPE, loc, id, type);
}

bool
Entry::typeDeclaration() const
{
    return kind == TYPE;
}

bool
Entry::variableDeclaration() const
{
    return kind == VAR;
}

bool
Entry::constantDeclaration() const
{
    return false;
}

bool
operator!=(const Entry &a, const Entry &b)
{
    if (a.kind != b.kind) {
	return false;
    }
    if (a.kind == Entry::VAR || a.kind == Entry::TYPE) {
	return a.type == b.type;
    }
    assert(0);
    return false;
}

} } // namespace symtab, abc

