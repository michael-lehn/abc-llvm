#ifndef SYMTAB_ENTRY
#define SYMTAB_ENTRY

#include "lexer/loc.hpp"
#include "type/type.hpp"

namespace abc { namespace symtab {

class Entry
{
    private:
	enum Kind {
	    VAR,
	    TYPE,
	};

	Entry(Kind kind, lexer::Loc loc, UStr id, const Type *type);

    public:
	static Entry createVarEntry(lexer::Loc loc, UStr id, const Type *type);
	static Entry createTypeEntry(lexer::Loc loc, UStr id, const Type *type);

	const Kind kind;
	const lexer::Loc loc;
	const UStr id;
	const Type *type;

	bool typeDeclaration() const;
	bool variableDeclaration() const;
	bool constantDeclaration() const;

	friend bool operator!=(const Entry &a, const Entry &b);
};

bool operator!=(const Entry &a, const Entry &b);

} } // namespace symtab, abc

#endif // SYMTAB_ENTRY
