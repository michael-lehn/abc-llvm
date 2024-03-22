#ifndef SYMTAB_ENTRY
#define SYMTAB_ENTRY

#include "expr/expr.hpp"
#include "lexer/loc.hpp"
#include "type/type.hpp"

namespace abc { namespace symtab {

class Entry
{
    private:
	enum Kind {
	    VAR,
	    TYPE,
	    EXPR,
	};

	Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type);
	Entry(lexer::Loc loc, UStr id, const Expr *expr);

	bool definitionFlag = false;

    public:
	static Entry createVarEntry(lexer::Loc loc, UStr id, const Type *type);
	static Entry createTypeEntry(lexer::Loc loc, UStr id, const Type *type);
	static Entry createExprEntry(lexer::Loc loc, UStr id, const Expr *expr);

	const Kind kind;
	const lexer::Loc loc;
	const UStr id;
	const Type *type;
	const Expr *expr;

	bool typeDeclaration() const;
	bool variableDeclaration() const;
	bool expressionDeclaration() const;

	bool setDefinitionFlag();

	friend bool operator!=(const Entry &a, const Entry &b);
};

bool operator!=(const Entry &a, const Entry &b);

} } // namespace symtab, abc

#endif // SYMTAB_ENTRY
