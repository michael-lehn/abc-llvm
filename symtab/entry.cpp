#include <cassert>

#include "entry.hpp"

namespace abc {
namespace symtab {

Entry::Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type)
    : id{id}, kind{kind}, loc{loc}, type{type}, expr{nullptr}
{
}

Entry::Entry(lexer::Loc loc, UStr id, const Expr *expr)
    : id{id}, kind{EXPR}, loc{loc}, type{nullptr}, expr{expr}
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

const UStr
Entry::getId() const
{
    return id;
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
Entry::setExternalLinkage()
{
    assert(variableDeclaration());
    // external declaration **can not** follow a static declaration
    if (linkage == INTERNAL_LINKAGE) {
	return false;
    }
    linkage = EXTERNAL_LINKAGE;
    return true;
}

bool
Entry::setInternalLinkage()
{
    assert(variableDeclaration());
    // static declaration **can not** follow a external declaration
    if (linkage == EXTERNAL_LINKAGE) {
	return false;
    }
    linkage = INTERNAL_LINKAGE;
    std::string prefix = ".";
    id = UStr::create(prefix + id.c_str());
    return true;
}

bool
Entry::setLinkage()
{
    assert(variableDeclaration());
    if (linkage == NO_LINKAGE) {
	// By default linkage is internal
	setInternalLinkage();
    }
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

} // namespace symtab
} // namespace abc
