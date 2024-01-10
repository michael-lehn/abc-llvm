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
    std::fprintf(stderr, "%zu.%zu-%zu.%zu: expected '%s' got '%s' (%s)\n",
	    token.loc.from.line, token.loc.from.col,
	    token.loc.to.line, token.loc.to.col,
	    s, token.val.c_str(), tokenCStr(token.kind));
    exit(1);
}

void
semanticError(const char *s)
{
    semanticError(token.loc, s);
}

void
semanticError(Token::Loc loc, const char *s)
{
    std::fprintf(stderr, "%zu.%zu-%zu.%zu: %s\n",
	    loc.from.line, loc.from.col,
	    loc.to.line, loc.to.col,
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
static bool parseGlobalDef(void);

void
parser(void)
{
    getToken();
    while (token.kind != TokenKind::EOI) {
	if (!parseGlobalDef() && !parseFn()) {
	    expectedError("function declaration, global variable definition"
			  " or EOF");
	}
    }
}

//------------------------------------------------------------------------------

static bool
parseGlobalDef(void)
{
    if (token.kind != TokenKind::GLOBAL) {
	return false;
    }
    getToken();

    do {
	expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    expectedError("type");
	}

	auto s = symtab::add(loc, ident.c_str(), type);
	if (!s) {
	    std::string msg = ident.c_str();
	    msg += " already defined";
	    semanticError(msg.c_str());
	}

	// parse initalizer
	ExprPtr init = nullptr;
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    init = parseConstExpr();
	    if (!init) {
		expectedError("non-empty constant expression");
	    }
	}
	auto initValue = init ? init->loadConst() : nullptr;
	gen::defGlobal(s->internalIdent.c_str(), s->type, initValue);
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	expected(TokenKind::COMMA);
	getToken();
    } while (true);

    expected(TokenKind::SEMICOLON);
    getToken();
    
    return true;
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
	auto loc = token.loc;
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
	    paramIdent->push_back(s->internalIdent.c_str());
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

const Type *
parseIntType(void)
{
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

const Type *
parsePtrType(void)
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto baseTy = parseType();
    if (!baseTy) {
	expectedError("base type for pointer");
    }
    return Type::getPointer(baseTy);
}

static const Type *
parseType(void)
{
    if (auto fnType = parseFnType()) {
	return fnType;
    } else if (auto ptrType = parsePtrType()) {
	return ptrType;
    }
    return parseIntType();
}

//------------------------------------------------------------------------------

static bool
parseLocalDef(void)
{
    if (token.kind != TokenKind::LOCAL) {
	return false;
    }
    getToken();

    do {
	expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    expectedError("type");
	}

	auto s = symtab::add(loc, ident.c_str(), type);
	if (!s) {
	    std::string msg = ident.c_str();
	    msg += " already defined";
	    semanticError(msg.c_str());
	}
	gen::defLocal(s->internalIdent.c_str(), s->type);

	// parse initalizer
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    auto init = parseExpr();
	    if (!init) {
		expectedError("non-empty expression");
	    }

	    auto var = Expr::createIdentifier(s->internalIdent.c_str(), type);
	    init = Expr::createBinary(Binary::Kind::ASSIGN,
				      std::move(var),
				      std::move(init));
	    init->loadValue();
	}
	if (token.kind != TokenKind::COMMA) {
	    return true;
	}
	expected(TokenKind::COMMA);
	getToken();
    } while (true);
}

static bool
parseLocalDefStmt(void)
{
    if (parseLocalDef()) {
	expected(TokenKind::SEMICOLON);
	getToken();
	return true;
    }
    return false;
}

//------------------------------------------------------------------------------

static bool parseStmt(void);

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
parseIfStmt(void)
{
    if (token.kind != TokenKind::IF) {
	return false;
    }
    getToken();

    // parse expr
    expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	expectedError("non-empty expression");
    }
    expected(TokenKind::RPAREN);
    getToken();

    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    expr->condJmp(thenLabel, elseLabel);
    
    // parse 'then' block
    gen::labelDef(thenLabel);
    if (!parseCompoundStmt(true)) {
	expectedError("compound statement block");
    }

    gen::jmp(endLabel);

    // parse optional 'else' block
    gen::labelDef(elseLabel);
    if (token.kind == TokenKind::ELSE) {
	getToken();
	if (!parseCompoundStmt(true) && !parseIfStmt()) {
	    expectedError("compound statement block or if statement");
	}
    }
    gen::jmp(endLabel); // connect with 'end' (even if 'else' is empyt)

    // end of 'then' and 'else' block
    gen::labelDef(endLabel);
    return true;
}

static bool
parseWhileStmt(void)
{
    if (token.kind != TokenKind::WHILE) {
	return false;
    }
    getToken();

    // parse expr
    expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	expectedError("non-empty expression");
    }
    expected(TokenKind::RPAREN);
    getToken();

    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    gen::jmp(condLabel);

    // 'while-cond' block
    gen::labelDef(condLabel);
    expr->condJmp(loopLabel, endLabel);
    
    // 'while-loop' block
    gen::labelDef(loopLabel);
    if (!parseCompoundStmt(true)) {
	expectedError("compound statement block");
    }
    gen::jmp(condLabel);

    // end of loop
    gen::labelDef(endLabel);
    return true;
}

static bool
parseForStmt(void)
{
    if (token.kind != TokenKind::FOR) {
	return false;
    }
    getToken();

    symtab::openScope();
    expected(TokenKind::LPAREN);
    getToken();
    // parse 'init': local definition or  expr
    if (!parseLocalDef()) {
	auto init = parseExpr();
	if (init) {
	    init->loadValue();
	}
    }
    expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'cond' expr
    auto cond = parseExpr();
    if (!cond) {
	cond = Expr::createLiteral("1", 10, nullptr);
    }
    expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'update' expr
    auto update = parseExpr();
    expected(TokenKind::RPAREN);
    getToken();


    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    gen::jmp(condLabel);

    // 'for-cond' block
    gen::labelDef(condLabel);
    cond->condJmp(loopLabel, endLabel);
    
    // 'for-loop' block
    gen::labelDef(loopLabel);
    if (!parseCompoundStmt(false)) {
	expectedError("compound statement block");
    }
    if (update) {
	update->loadValue();
    }
    gen::jmp(condLabel);

    // end of loop
    gen::labelDef(endLabel);
    return true;
}


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
    gen::ret(expr->loadValue());
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
    expr->loadValue();
    return true;
}

static bool
parseStmt(void)
{
    return parseCompoundStmt(true)
	|| parseIfStmt()
	|| parseWhileStmt()
	|| parseForStmt()
	|| parseReturnStmt()
	|| parseExprStmt()
	|| parseLocalDefStmt();
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

    symtab::closeScope();
    return true;
}


