#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "error.hpp"
#include "gen.hpp"
#include "lexer.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "ustr.hpp"

static bool parseFn(void);
static bool parseGlobalDef(void);

void
parser(void)
{
    getToken();
    while (token.kind != TokenKind::EOI) {
	if (!parseGlobalDef() && !parseFn()) {
	    error::out() << token.loc
		<< "function declaration, global variable definition or EOF"
		<< std::endl;
	    error::fatal();
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
	error::expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	error::expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected" << std::endl;
	    error::fatal();
	}

	// TODO: Provide symtab::defGlobal(...) ?
	auto s = symtab::addToRootScope(loc, ident.c_str(), type);
	auto ty = s->type;

	// parse initalizer
	ExprPtr init = nullptr;
	if (token.kind == TokenKind::EQUAL) {
	    auto opLoc = token.loc;
	    getToken();
	    init = parseConstExpr();
	    if (!init) {
		error::out() << token.loc
		    << " expected non-empty constant expression" << std::endl;
		error::fatal();
	    }
	    auto toTy = Type::getTypeConversion(init->getType(), ty, opLoc);
	    if (toTy != ty) {
		init->print();
		error::out() << opLoc << " can not cast expression of type '"
		    << init->getType() << "' to type '" << ty
		    << "'" <<std::endl;
		error::fatal();
	    } else if (toTy != init->getType()) {
	    	init = Expr::createCast(std::move(init), toTy, loc);
	    }
	}
	auto initValue = init ? init->loadConst() : nullptr;
	gen::defGlobal(s->internalIdent.c_str(), ty, initValue);
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	error::expected(TokenKind::COMMA);
	getToken();
    } while (true);

    error::expected(TokenKind::SEMICOLON);
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

	error::expected(TokenKind::COLON);
	getToken();

	// parse param type
	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected"
		<< std::endl;
	    error::fatal();
	}
	argType.push_back(type);

	// add param to symtab if this is a declaration
	if (paramIdent) {
	    auto s = symtab::add(loc, ident.c_str(), type);
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
	error::expected(TokenKind::IDENTIFIER);
    }
    if (token.kind == TokenKind::IDENTIFIER) {
	fnLoc = token.loc;
	fnIdent = token.val.c_str();
	getToken();
    }

    // parse function parameters
    error::expected(TokenKind::LPAREN);
    getToken();
    if (fnDecl) {
	symtab::openScope();
	parseFnParamDecl(argType, *fnParamIdent);
    } else {
	parseFnParamType(argType);
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    // parse function return type
    retType = Type::getVoid();
    if (token.kind == TokenKind::COLON) {
	getToken();
	retType = parseType();
    }

    auto fnType = Type::getFunction(retType, argType);
    
    if (fnDecl) {
	*fnDecl = symtab::addToRootScope(fnLoc, fnIdent.c_str(), fnType);
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

static const Type *
parsePtrType(void)
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto baseTy = parseType();
    if (!baseTy) {
	error::out() << token.loc << " expected base type for pointer"
	    << std::endl;
	error::fatal();
    }
    return Type::getPointer(baseTy);
}

static const Type *
parseArrayDimAndType(void)
{
    if (token.kind != TokenKind::LBRACKET) {
	return nullptr;
    }
    getToken();
    auto dim = parseExpr();
    if (!dim || !dim->isConst() || !dim->getType()->isInteger()) {
	error::out() << token.loc
	    << " dimension has to be a constant integer expression"
	    << std::endl;
	error::fatal();
    }
    auto dimVal = llvm::dyn_cast<llvm::ConstantInt>(dim->loadConst());

    if (dimVal->isNegative()) {
	error::out() << token.loc
	    << " dimension can not be negative"
	    << std::endl;
	error::fatal();
    }

    error::expected(TokenKind::RBRACKET);
    getToken();

    const Type *ty = nullptr;
    if (token.kind == TokenKind::OF) {
	getToken();
	ty = parseType();
    } else {
	ty = parseArrayDimAndType();
    }
    if (!ty) {
	error::out() << token.loc << " expected element type"
	    << std::endl;
	error::fatal();
    }
    return Type::getArray(ty, dimVal->getZExtValue());
}

static const Type *
parseArrayType(void)
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    return parseArrayDimAndType();
}

const Type *
parseType(void)
{
    if (auto fnType = parseFnType()) {
	return fnType;
    } else if (auto ptrType = parsePtrType()) {
	return ptrType;
    } else if (auto arrayType = parseArrayType()) {
	return arrayType;
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
	error::expected(TokenKind::IDENTIFIER);
	auto loc = token.loc;
	auto ident = token.val;
	getToken();

	error::expected(TokenKind::COLON);
	getToken();

	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << " type expected"
		<< std::endl;
	    error::fatal();
	}

	auto s = symtab::add(loc, ident.c_str(), type);
	gen::defLocal(s->internalIdent.c_str(), s->type);

	// parse initalizer
	if (token.kind == TokenKind::EQUAL) {
	    auto opLoc = token.loc;
	    getToken();
	    auto init = parseExpr();
	    if (!init) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
	    }

	    auto var = Expr::createIdentifier(ident.c_str(), loc);
	    init = Expr::createBinary(Binary::Kind::ASSIGN,
				      std::move(var),
				      std::move(init),
				      opLoc);
	    init->loadValue();
	}
	if (token.kind != TokenKind::COMMA) {
	    return true;
	}
	error::expected(TokenKind::COMMA);
	getToken();
    } while (true);
}

static bool
parseLocalDefStmt(void)
{
    if (parseLocalDef()) {
	error::expected(TokenKind::SEMICOLON);
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

    error::expected(TokenKind::RBRACE);
    getToken();
    symtab::closeScope();
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
    error::expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	error::out() << token.loc << " expected non-empty expression"
	    << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::RPAREN);
    getToken();

    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    expr->condJmp(thenLabel, elseLabel);
    
    // parse 'then' block
    gen::labelDef(thenLabel);
    if (!parseCompoundStmt(true)) {
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
    }

    gen::jmp(endLabel);

    // parse optional 'else' block
    gen::labelDef(elseLabel);
    if (token.kind == TokenKind::ELSE) {
	getToken();
	if (!parseCompoundStmt(true) && !parseIfStmt()) {
	    error::out() << token.loc
		<< " compound statement block or if statement" << std::endl;
	    error::fatal();
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
    error::expected(TokenKind::LPAREN);
    getToken();
    auto expr = parseExpr();
    if (!expr) {
	error::out() << token.loc << " expected non-empty expression"
	    << std::endl;
	error::fatal();
    }
    error::expected(TokenKind::RPAREN);
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
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
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
    error::expected(TokenKind::LPAREN);
    getToken();
    // parse 'init': local definition or  expr
    if (!parseLocalDef()) {
	auto init = parseExpr();
	if (init) {
	    init->loadValue();
	}
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'cond' expr
    auto cond = parseExpr();
    if (!cond) {
	cond = Expr::createLiteral("1", 10, nullptr);
    }
    error::expected(TokenKind::SEMICOLON);
    getToken();
    // parse 'update' expr
    auto update = parseExpr();
    error::expected(TokenKind::RPAREN);
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
	error::out() << token.loc << " expected compound statement block"
	    << std::endl;
	error::fatal();
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
    error::expected(TokenKind::SEMICOLON);
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
    error::expected(TokenKind::SEMICOLON);
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
	symtab::closeScope();
    } else {
	error::expected(TokenKind::LBRACE);
	gen::fnDef(fnDecl->ident.c_str(), fnDecl->type, fnParamIdent);
	assert(parseCompoundStmt(false));
	gen::fnDefEnd();
    }

    return true;
}


