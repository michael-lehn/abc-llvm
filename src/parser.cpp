#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "gen.hpp"
#include "lexer.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "ustr.hpp"

void
expectedError(const char *s)
{
    std::fprintf(stderr, "%zu.%zu-%zu.%zu: expected '%s' got '%s'\n",
	    token.loc.from.line, token.loc.from.col,
	    token.loc.to.line, token.loc.to.col,
	    s, tokenCStr(token.kind));
    exit(1);
}

void
semanticError(const char *s)
{
    std::fprintf(stderr, "%zu.%zu-%zu.%zu: %s\n",
	    token.loc.from.line, token.loc.from.col,
	    token.loc.to.line, token.loc.to.col,
	    s);
    exit(1);
}


void
expected(TokenKind kind)
{
    if (token.kind != kind) {
	expectedError(tokenCStr(kind));
    }
}


static const Type *parseType(void);
static bool parseFn(void);

void
parser(void)
{
    getToken();
    while (token.kind != TokenKind::EOI) {
	if (!parseFn()) {
	    expectedError("function declaration or EOF");
	}
    }
}

//------------------------------------------------------------------------------

static void
parseFnParamDeclOrType(std::vector<const Type *> &argType,
		       std::vector<const char *> *paramIdent = nullptr)
{
    argType.clear();
    if (paramIdent) {
	paramIdent->clear();
    }

    while (token.kind == TokenKind::IDENTIFIER
        || token.kind == TokenKind::COLON)
    {
	// if parameter has no identifier give it an interal identifier 
	UStr ident = ".param";
	Token::Loc loc = token.loc;
	if (token.kind == TokenKind::IDENTIFIER) {
	    ident = token.val.c_str();
	    getToken();
	}

	expected(TokenKind::COLON);
	getToken();

	// parse param type
	auto type = parseType();
	if (!type) {
	    expectedError("type");
	}
	argType.push_back(type);

	// add param to symtab if this is a declaration
	if (paramIdent) {
	    auto s = symtab::add(loc, ident.c_str(), type);
	    if (!s) {
		std::string msg = ident.c_str();
		msg += " already defined";
		semanticError(msg.c_str());
	    }
	    paramIdent->push_back(s->ident.c_str());
	}

	// done if we don't get a COMMA
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
}

static void
parseFnParamDecl(std::vector<const Type *> &argType,
		 std::vector<const char *> &paramIdent)
{
    parseFnParamDeclOrType(argType, &paramIdent);
}

static void
parseFnParamType(std::vector<const Type *> &argType)
{
    parseFnParamDeclOrType(argType);
}

//------------------------------------------------------------------------------

static const Type *
parseFnDeclOrType(std::vector<const Type *> &argType, const Type *&retType,
	          symtab::SymEntry **fnDecl = nullptr,
		  std::vector<const char *> *fnParamIdent = nullptr)
{
    UStr fnIdent;
    Token::Loc fnLoc;

    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();

    // parse function identifier
    if (fnDecl) {
	expected(TokenKind::IDENTIFIER);
    }
    if (token.kind == TokenKind::IDENTIFIER) {
	fnLoc = token.loc;
	fnIdent = token.val.c_str();
	getToken();
    }

    // parse function parameters
    expected(TokenKind::LPAREN);
    getToken();
    if (fnDecl) {
	symtab::openScope();
	parseFnParamDecl(argType, *fnParamIdent);
    } else {
	parseFnParamType(argType);
    }
    expected(TokenKind::RPAREN);
    getToken();

    // parse function return type
    retType = nullptr;
    if (token.kind == TokenKind::COLON) {
	getToken();
	retType = parseType();
    }
    
    auto fnType = Type::getFunction(retType, argType);
    
    if (fnDecl) {
	*fnDecl = symtab::get(fnIdent.c_str(), symtab::RootScope);
	if (!*fnDecl) {
	    // TODO: in this case *fnDecl must not be a definition but a decl
	    *fnDecl = symtab::addToRootScope(fnLoc, fnIdent.c_str(), fnType);
	}
	if (!*fnDecl) {
	    std::string msg = fnIdent.c_str();
	    msg += " already defined";
	    semanticError(msg.c_str());
	}
    }

    return fnType;
}

static symtab::SymEntry *
parseFnDecl(std::vector<const char *> &fnParamIdent)
{
    std::vector<const Type *> argType;
    const Type *retType;
    symtab::SymEntry *fnDecl;

    parseFnDeclOrType(argType, retType, &fnDecl, &fnParamIdent);
    return fnDecl;
}

static const Type *
parseFnType(void)
{
    std::vector<const Type *> argType;
    const Type *retType;

    return parseFnDeclOrType(argType, retType);
}

//------------------------------------------------------------------------------

static const Type *
parseType(void)
{
    if (auto fnType = parseFnType()) {
	return fnType;
    }
    switch (token.kind) {
	case TokenKind::U8:
	    getToken();
	    return Type::getUnsignedInteger(8);
	case TokenKind::U16:
	    getToken();
	    return Type::getUnsignedInteger(16);
	case TokenKind::U32:
	    getToken();
	    return Type::getUnsignedInteger(32);
	case TokenKind::U64:
	    getToken();
	    return Type::getUnsignedInteger(64);
	case TokenKind::I8:
	    getToken();
	    return Type::getSignedInteger(8);
	case TokenKind::I16:
	    getToken();
	    return Type::getSignedInteger(16);
	case TokenKind::I32:
	    getToken();
	    return Type::getSignedInteger(32);
	case TokenKind::I64:
	    getToken();
	    return Type::getSignedInteger(64);
	default:
	    return nullptr;		
    }
}

//------------------------------------------------------------------------------

static bool parseStmt(void);

static bool
parseReturnStmt(void)
{
    if (token.kind != TokenKind::RETURN) {
	return false;
    }
    getToken();
    auto expr = parseExpr();
    expected(TokenKind::SEMICOLON);
    getToken();
    gen::ret(load(expr.get()));
    return true;
}

static bool
parseExprStmt(void)
{
    auto expr = parseExpr();
    if (!expr) {
	return false;
    }
    expected(TokenKind::SEMICOLON);
    getToken();
    load(expr.get());
    return true;
}

static bool
parseCompoundStmt(bool openScope)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    if (openScope) {
	symtab::openScope();
    }

    while (parseStmt()) {
    }

    expected(TokenKind::RBRACE);
    getToken();
    if (openScope) {
	symtab::closeScope();
    }
    return true;
}

static bool
parseStmt(void)
{
    return parseCompoundStmt(true)
	|| parseReturnStmt()
	|| parseExprStmt();
}

//------------------------------------------------------------------------------

static bool
parseFn(void)
{
    std::vector<const char *> fnParamIdent;
    auto fnDecl = parseFnDecl(fnParamIdent);
    if (!fnDecl) {
	return false;
    }

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	gen::fnDecl(fnDecl->ident.c_str(), fnDecl->type);
    } else {
	expected(TokenKind::LBRACE);
	gen::fnDef(fnDecl->ident.c_str(), fnDecl->type, fnParamIdent);
	assert(parseCompoundStmt(false));
	gen::fnDefEnd();
    }

    symtab::print(std::cout);
    symtab::closeScope();
    symtab::print(std::cout);
    return true;
}


