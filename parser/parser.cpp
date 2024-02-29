#include <iostream>

#include "expr/expr.hpp"
#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"
#include "type/functiontype.hpp"
#include "type/voidtype.hpp"
#include "defaulttype.hpp"
#include "parser.hpp"

namespace abc {

using namespace lexer;

//------------------------------------------------------------------------------
static AstPtr parseTopLevelDeclaration();

/*
 * input-sequence = {top-level-declaration} EOI
 */
AstPtr
parser()
{
    Symtab newScope;
    initDefaultType();
    getToken();

    auto top = std::make_unique<AstList>();
    while (auto decl = parseTopLevelDeclaration()) {
	top->append(std::move(decl));
    }
    if (token.kind != TokenKind::EOI) {
	error::out() << token.loc << ": error: unexpected "
	    << token.val.c_str() << std::endl;
	return nullptr;
    }
    getToken();
    return top;
}

//------------------------------------------------------------------------------
static AstPtr parseFunctionDeclarationOrDefinition();
static AstPtr parseExternDeclaration();

/*
 * top-level-declaration = function-declaration-or-definition
 *			 | extern-declaration
 *			 | global-variable-definition
 *			 | type-declaration
 *			 | struct-declaration
 *			 | enum-declaration
 */
static AstPtr
parseTopLevelDeclaration()
{
    AstPtr ast;

    (ast = parseFunctionDeclarationOrDefinition())
    || (ast = parseExternDeclaration());

    return ast;
}

//------------------------------------------------------------------------------
static const Type *parseFunctionType(Token &fnName,
				     std::vector<Token> &fnParamName);
static AstPtr parseFunctionBody();

/*
 * function-declaration-or-definition
 *	= function-type (";" | compound-statement)
 */
static AstPtr
parseFunctionDeclarationOrDefinition()
{
    Token fnName;
    std::vector<Token> fnParamName;

    const Type *fnType = parseFunctionType(fnName, fnParamName);
    if (!fnType) {
	return nullptr;
    }

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	return std::make_unique<AstFuncDecl>(fnName,
					     fnType,
					     std::move(fnParamName),
					     false);
    }

    auto fnDef = std::make_unique<AstFuncDef>(fnName, fnType);

    Symtab newScope(fnName.val);
    fnDef->appendParamName(std::move(fnParamName));

    auto fnBody = parseFunctionBody();
    if (!fnBody) {
	error::out() << token.loc << ": expected ';' or compound statement"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    fnDef->appendBody(std::move(fnBody));
    return fnDef;
}

//------------------------------------------------------------------------------
static bool parseFunctionParameterList(std::vector<Token> &paramName,
				       std::vector<const Type *> &paramType,
				       bool &hasVarg);

/*
 * function-type
 *	= "fn" [identifier] "(" function-parameter-list ")" [ ":" type ]
 */
static const Type *
parseFunctionType(Token &fnName, std::vector<Token> &fnParamName)
{
    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();
    if (token.kind == TokenKind::IDENTIFIER) {
	fnName = token;
	getToken();
    }
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();

    std::vector<const Type *> fnParamType;
    bool hasVarg = false;
    if (!parseFunctionParameterList(fnParamName, fnParamType, hasVarg)) {
	error::out() << token.loc << ": expected parameter list" << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto fnRetType = VoidType::create();
    if (token.kind == TokenKind::COLON) {
	getToken();
	fnRetType = parseType();
	if (!fnRetType) {
	    error::out() << token.loc << ": expected return type" << std::endl;
	    error::fatal();
	    return nullptr;
	}
    }
    return FunctionType::create(fnRetType, std::move(fnParamType), hasVarg);
}

//------------------------------------------------------------------------------
/*
 * function-parameter-list
 *	= [ [identifier] ":" type { "," [identifier] ":" type} } ["," ...] ]
 */
static bool
parseFunctionParameterList(std::vector<Token> &paramName,
			   std::vector<const Type *> &paramType,
			   bool &hasVarg)
{
    if (token.kind != TokenKind::IDENTIFIER && token.kind != TokenKind::COLON) {
	return true; // empty parameter list
    }
    hasVarg = false;
    while (true) {
	if (token.kind == TokenKind::IDENTIFIER) {
	    paramName.push_back(token);
	    getToken();
	} else {
	    paramName.push_back(Token{});
	}
	if (!error::expected(TokenKind::COLON)) {
	    return false;
	}
	getToken();
	auto ty = parseType();
	if (!ty) {
	    error::out() << token.loc
		<< ": expected parameter type" << std::endl;
	    error::fatal();
	    return false;
	}
	paramType.push_back(ty);
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
	if (token.kind == TokenKind::DOT3) {
	    getToken();
	    hasVarg = true;
	    break;
	}
    }
    return true;
}

//------------------------------------------------------------------------------
static const Type *parseFunctionDeclaration(Token &fnIdent,
					    std::vector<Token> &param);
static AstListPtr parseExternVariableDeclaration();

/*
 * extern-declaration
 *	= "extern" ( function-declaration | extern-variable-declaration ) ";"
 */
static AstPtr
parseExternDeclaration()
{
    if (token.kind != TokenKind::EXTERN) {
	return nullptr;
    }
    getToken();

    Token fnIdent;
    std::vector<Token> fnParamName;
    auto fnType = parseFunctionDeclaration(fnIdent, fnParamName);
    auto varDecl = parseExternVariableDeclaration();

    if (!fnType && !varDecl) {
	error::out() << token.loc
	    << ": error: expected extern function or variable declaration"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    if (fnType) {
	return std::make_unique<AstFuncDecl>(fnIdent,
					     fnType,
					     std::move(fnParamName),
					     true);
    } else if (varDecl) {
	return std::make_unique<AstExternVar>(std::move(varDecl));
    }
    assert(0);
    return nullptr;
}

//------------------------------------------------------------------------------
/*
 * function-declaration = function-type
 */
static const Type *
parseFunctionDeclaration(Token &fnIdent, std::vector<Token> &param)
{
    return parseFunctionType(fnIdent, param);
}

//------------------------------------------------------------------------------
static AstPtr parseStatementOrDeclarationList();

/*
 * compound-statement = "{" statement-or-declaration-list "}"
 */

static AstPtr
parseFunctionBody()
{
    if (token.kind != TokenKind::LBRACE) {
	return nullptr;
    }
    getToken();

    auto body = parseStatementOrDeclarationList();

    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return body;
}

//------------------------------------------------------------------------------
static const Type *parseUnqualifiedType();

/*
 * type = [const] unqualified-type
 */
const Type *
parseType()
{
    if (token.kind == TokenKind::CONST) {
	getToken();
	auto type = parseUnqualifiedType();
	if (!type) {
	    error::out() << token.loc << ": unqualified type expected"
		<< std::endl;
	    error::fatal();
	    return nullptr;
	}
	return type->getConst();
    } else if (auto type = parseUnqualifiedType()) {
	return type;
    } else {
	return nullptr;
    }
}

//------------------------------------------------------------------------------
//static const Type *parsePointerType();
//static const Type *parseArrayType();

/*
 * unqualified-type = identifier
 *		    | pointer-type
 *		    | array-type
 *		    | function-type
 */
static const Type *
parseUnqualifiedType()
{
    if (token.kind == TokenKind::IDENTIFIER) {
	if (auto entry = Symtab::type(token.val, Symtab::AnyScope)) {
	    getToken();
	    return entry->type;
	}
	return nullptr;
    /*
    } else if (auto type = parsePointerType()) {
	return type;
    } else if (auto type = parseArrayType()) {
	return type;
    } else if (auto type = parseFunctionType()) {
	return type;
    */
    } else {
	return nullptr;
    }
}


//------------------------------------------------------------------------------

/*
 * statement-or-declaration = "{" { statement | declaration } "}"
 */

static AstPtr
parseStatementOrDeclarationList()
{
    return std::make_unique<AstList>();
}

//------------------------------------------------------------------------------
/*
 * extern-variable-declaration = identifier ":" type { "," identifier ":" type }
 */
static AstListPtr
parseExternVariableDeclaration()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return nullptr;
    }
    
    auto astList = std::make_unique<AstList>();

    while (token.kind == TokenKind::IDENTIFIER) {
	auto varName = token;
	getToken();
	if (!error::expected(TokenKind::COLON)) {
	    return nullptr;
	}
	getToken();
	auto varType = parseType();
	if (!varType) {
	    error::out() << token.loc << ": expected variable type"
		<< std::endl;
	    error::fatal();
	    return nullptr;
	}
	astList->append(std::make_unique<AstVar>(varName, varType));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return astList;
}

} // namespace abc
