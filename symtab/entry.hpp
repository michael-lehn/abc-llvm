#ifndef SYMTAB_ENTRY
#define SYMTAB_ENTRY

#include "expr/expr.hpp"
#include "lexer/loc.hpp"
#include "type/type.hpp"

namespace abc {
namespace symtab {

class Entry
{
    private:
	enum Kind
	{
	    VAR,
	    TYPE,
	    EXPR,
	};

	enum Linkage
	{
	    NO_LINKAGE,
	    EXTERNAL_LINKAGE,
	    INTERNAL_LINKAGE,
	};

	Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type);
	Entry(lexer::Loc loc, UStr id, const Expr *expr);

	bool definitionFlag = false;
	Linkage linkage = NO_LINKAGE;
	UStr id;

    public:
	static Entry createVarEntry(lexer::Loc loc, UStr id, const Type *type);
	static Entry createTypeEntry(lexer::Loc loc, UStr id, const Type *type);
	static Entry createExprEntry(lexer::Loc loc, UStr id, const Expr *expr);

	const Kind kind;
	const lexer::Loc loc;

	const Type *type;
	const Expr *expr;

	const UStr getId() const;

	bool typeDeclaration() const;
	bool variableDeclaration() const;
	bool expressionDeclaration() const;

	bool setDefinitionFlag();
	bool setExternalLinkage();
	bool setInternalLinkage();
	bool setLinkage();

	friend bool operator!=(const Entry &a, const Entry &b);
};

bool operator!=(const Entry &a, const Entry &b);

} // namespace symtab
} // namespace abc

#endif // SYMTAB_ENTRY
