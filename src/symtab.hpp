#ifndef SYMTAB_HPP
#define SYMTAB_HPP

#include <iostream>
#include <variant>

#include "expr.hpp"
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

	enum Linkage {
	    Internal,
	    Extern,
	};
	
	struct Entry
	{
	    friend Symtab; // Entry can only be created through Symtab methods

	    public:
		const UStr ident;

		Token::Loc getLoc() const
		{
		    return hasDefinitionFlag() ? loc : firstDeclLoc;
		}

		bool holdsExpr() const
		{
		    return std::holds_alternative<ExprPtr>(data);
		}

		const Expr *expr() const
		{
		    assert(std::holds_alternative<ExprPtr>(data));
		    return std::get<ExprPtr>(data).get();
		}

		const Type *type() const
		{
		    if (std::holds_alternative<const Type *>(data)) {
			return std::get<const Type *>(data);
		    } else if (std::holds_alternative<ExprPtr>(data)) {
			const auto &e = std::get<ExprPtr>(data);
			return e->type;
		    }
		    assert(0);
		    return nullptr;
		}

		bool hasExternFlag() const
		{
		    return externFlag;
		}

		void setExternFlag()
		{
		    externFlag = true;
		}


		bool hasDefinitionFlag() const
		{
		    return definition;
		}

		// returns 'false' iff symbol was alreay defined.
		bool setDefinitionFlag();

		// returns an identifier that can be used for code generation
		UStr getInternalIdent() const
		{
		    return internalIdent;
		}

		void invalidate()
		{
		    *(char *)&internalIdent = 0;
		    data = nullptr;
		    loc = firstDeclLoc = lastDeclLoc = Token::Loc{};
		}
	
	    private:
		using Data = std::variant<const Type *, ExprPtr>;

		Entry(Token::Loc loc, Data &&data, UStr ident,
		      UStr internalIdent)
		    : ident{ident}, loc{loc}, firstDeclLoc{loc}
		    , lastDeclLoc{loc} , data{std::move(data)}
		    , internalIdent{internalIdent} , definition{false}
		    , externFlag{false}
		{
		}

		Token::Loc loc, firstDeclLoc, lastDeclLoc;
		Data data;
		UStr internalIdent;
		bool definition;
		bool externFlag;
	};

	// Used to create unique identifieriers within functions scope.
	static void setPrefix(UStr prefix);

	static void openScope();
	static void closeScope();

	static Entry *get(UStr ident, Scope scope = AnyScope);

    private:
	static Symtab::Entry *get(UStr ident, ScopeNode *begin, ScopeNode *end);
	static Entry *add(Token::Loc loc, UStr ident, Entry::Data &&data,
			  ScopeNode *sn);

    public:
	// Add a new symbol to current scope. Returns pointer to existing
	// or created entry
	static Entry *addDecl(Token::Loc loc, UStr ident, const Type *type);

	static Entry *addConstant(Token::Loc loc, UStr ident);
	static Entry *addConstant(Token::Loc loc, UStr ident, ExprPtr &&val);

	// Add a new symbol to current scope. Returns pointer to existing
	// or created entry
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
	static const Type *addTypeAlias(UStr ident, const Type *type,
					Token::Loc loc = Token::Loc{});


	static std::ostream &print(std::ostream &out);

};

#endif // SYMTAB_HPP
