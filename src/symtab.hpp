#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include <ostream>

#include "lexer.hpp"
#include "type.hpp"
#include "ustr.hpp"

struct ScopeNode;

class Symtab
{
    public:

	enum Scope {
	    AnyScope,
	    CurrentScope,
	    RootScope,
	};
	
	struct Entry
	{
	    friend Symtab; // Entry can only be created through Symtab methods

	    public:
		const UStr ident;

		Token::Loc getLoc(void) const
		{
		    return hasDefinitionFlag() ? loc : lastDeclLoc;
		}

		const Type *getType()
		{
		    return type;
		}

		bool hasDefinitionFlag() const
		{
		    return definition_;
		}

		// returns 'false' iff symbol was alreay defined.
		bool setDefinitionFlag(void);

		// returns an identifier that can be used for code generation
		UStr internalIdent(void) const
		{
		    return internalIdent_;
		}
	
	    private:
		Entry(Token::Loc loc, const Type *type, UStr ident,
		      UStr internalIdent)
		    : ident{ident}, loc{loc}, type{type}
		    , internalIdent_{internalIdent}, definition_{false}
		{}

		Token::Loc loc, lastDeclLoc;
		const Type *type;
		UStr internalIdent_;
		bool definition_;
	};

	static void openScope(void);
	static void closeScope(void);

	static Entry *get(UStr ident, Scope scope = AnyScope);

    private:
	static Symtab::Entry *get(UStr ident, ScopeNode *begin, ScopeNode *end);
	static Entry *add(Token::Loc loc, UStr ident, const Type *type,
			  ScopeNode *sn);

    public:
	// Add a new symbol to current scope. Returns nullptr if symbol already
	// exists, otherwise returns a pointer to the created entry.
	static Entry *addDecl(Token::Loc loc, UStr ident, const Type *type);

	// Add a new symbol to root scope. Returns nullptr if symbol already
	// exists, otherwise a pointer to the created entry.
	static Entry *addDeclToRootScope(Token::Loc loc, UStr ident,
					 const Type *type);

	// returns an identifier for the string literal 'str'
	static UStr addStringLiteral(UStr str);

	// handle named types
    private:
	static const Type *getNamedType(UStr name, ScopeNode *b, ScopeNode *e);

    public:
	static const Type *getNamedType(UStr ident, Scope scope);

	// returns nullptr if 'ident' was found in current type scope,
	// otherwise returns 'type'.
	//
	// When 'ident' goes out of scope the type
	// interface gets notified and can delete the type
	static const Type *addNamedType(UStr ident, const Type *type);

	// returns nullptr if 'ident' was found in current type scope,
	// otherwise returns 'type'.
	static const Type *addTypeAlias(UStr ident, const Type *type);

	static std::ostream &print(std::ostream &out);

};

#endif // SYMTAB_HPP
