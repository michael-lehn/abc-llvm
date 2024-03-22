#include <cassert>
#include <iostream>

#include "entry.hpp"

namespace abc { namespace symtab {

Entry::Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type)
    : kind{kind}, loc{loc}, id{id}, type{type}, expr{nullptr}
{
}

Entry::Entry(lexer::Loc loc, UStr id, const Expr *expr)
    : kind{EXPR}, loc{loc}, id{id}, type{nullptr}, expr{expr}
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

Entry
Entry::createExprEntry(lexer::Loc loc, UStr id, const Expr *expr)
{
    return Entry(loc, id, expr);
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
Entry::expressionDeclaration() const
{
    return kind == EXPR;
}

bool
Entry::setDefinitionFlag()
{
    assert(variableDeclaration());
    if (definitionFlag) {
	return false;
    }
    definitionFlag = true;
    return true;
}

bool
operator!=(const Entry &a, const Entry &b)
{
    if (a.kind != b.kind) {
	return true;
    }
    if (a.kind == Entry::VAR || a.kind == Entry::TYPE) {
	return a.type != b.type;
    }
    if (a.kind == Entry::EXPR) {
	return a.expr != b.expr;
    }
    assert(0);
    return false;
}

} } // namespace symtab, abc

