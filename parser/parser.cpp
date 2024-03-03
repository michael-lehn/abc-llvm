#include <iostream>
#include <sstream>

#include "expr/expr.hpp"
#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"
#include "type/functiontype.hpp"
#include "type/integertype.hpp"
#include "type/voidtype.hpp"

#include "defaulttype.hpp"
#include "parseexpr.hpp"
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
static AstPtr parseGlobalVariableDefinition();
static AstPtr parseEnumDeclaration();

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
    || (ast = parseExternDeclaration())
    || (ast = parseGlobalVariableDefinition())
    || (ast = parseEnumDeclaration());

    return ast;
}

//------------------------------------------------------------------------------
static const Type *parseFunctionType(Token &fnName,
				     std::vector<Token> &fnParamName);
static AstPtr parseFunctionBody();

/*
 * function-declaration-or-definition
 *	= function-type (";" | function-body)
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
    std::size_t unusedCount = 0;
    while (true) {
	if (token.kind == TokenKind::IDENTIFIER) {
	    paramName.push_back(token);
	    getToken();
	} else {
	    std::stringstream ss;
	    ss << ".unused" << unusedCount++;
	    auto unused = UStr::create(ss.str());
	    paramName.push_back(Token{token.loc, token.kind, unused});
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
 * function-body = "{" statement-or-declaration-list "}"
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
static AstPtr parseDeclaration();
static AstPtr parseStatement();

/*
 * statement-or-declaration = "{" { statement | declaration } "}"
 */

static AstPtr
parseStatementOrDeclarationList()
{
    auto list = std::make_unique<AstList>();
    for (AstPtr ast; (ast = parseStatement()) || (ast = parseDeclaration());) {
	list->append(std::move(ast));
    }
    return list;
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

//------------------------------------------------------------------------------
static AstPtr parseGlobalVariableDeclaration();

/*
 * global-variable-definition = global-variable-declaration ";"
 */
static AstPtr
parseGlobalVariableDefinition()
{
    auto def = parseGlobalVariableDeclaration();
    if (!def) {
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return def;
}

//------------------------------------------------------------------------------
static AstListPtr parseVariableDeclarationList();

/*
 * global-variable-declaration = "global" variable-declaration-list
 */
static AstPtr
parseGlobalVariableDeclaration()
{
    if (token.kind != TokenKind::GLOBAL) {
	return nullptr;
    }
    getToken();
    auto def = parseVariableDeclarationList();
    if (!def) {
	error::out() << token.loc
	    << ": expected global variable declaration list" << std::endl;
	error::fatal();
	return nullptr;
    }
    return std::make_unique<AstGlobalVar>(std::move(def));
}

//------------------------------------------------------------------------------
static AstVarPtr parseVariableDeclaration();

/*
 * variable-declaration-list = variable-declaration { "," variable-declaration }
 */
static AstListPtr
parseVariableDeclarationList()
{
    auto astList = std::make_unique<AstList>();

    for (bool first = true; ; first = false) {
	auto decl = parseVariableDeclaration();
	if (!decl) {
	    if (!first) {
		error::out() << token.loc << ": expected variable declaration"
		    << std::endl;
		error::fatal();
	    }
	    return nullptr;
	}
	astList->append(std::move(decl));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return astList;
}

//------------------------------------------------------------------------------
static AstPtr parseInitializer(const Type *type);

/*
 * variable-declaration = identifier ":" type [ "=" initializer ]
 */
static AstVarPtr
parseVariableDeclaration()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return nullptr;
    }
    auto varName = token;
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return nullptr;
    }
    getToken();
    auto varType = parseType();
    if (!varType) {
	error::out() << token.loc << ": expected variable type" << std::endl;
	error::fatal();
	return nullptr;
    }
    auto astVar = std::make_unique<AstVar>(varName, varType);
    if (token.kind == TokenKind::EQUAL) {
	getToken();
	auto initializer = parseInitializer(varType);
	if (!initializer) {
	    error::out() << token.loc << ": initializer expected" << std::endl;
	    error::fatal();
	    return nullptr;
	}
	assert(0 && "TODO: astVar->addInitializer(std::move(initializer));");
    }
    return astVar;
}

//------------------------------------------------------------------------------
/*
 * initializer = expression
 *             | initializer-list
 */
static AstPtr
parseInitializer(const Type *type)
{
    assert(0 && "TODO: parseInitializer(const Type *type)");
    return nullptr;
}

//------------------------------------------------------------------------------
static AstPtr parseLocalVariableDefinition();

/*
 * declaration = type-declaration
 *	       | enum-declaration
 *	       | struct-declaration
 *	       | global-variable-definition
 *	       | local-variable-definition
 */
static AstPtr
parseDeclaration()
{
    AstPtr ast;

    (ast = parseLocalVariableDefinition())
	|| (ast = parseEnumDeclaration());

    /*
    (ast = parseTypeDeclaration())
	|| (ast = parseEnumDeclaration())
	|| (ast = parseStructDeclaration())
	|| (ast = parseGlobalVariableDefinition())
	|| (ast = parseLocalVariableDefinition());
	*/
    return ast;
}

//------------------------------------------------------------------------------
static AstPtr parseLocalVariableDeclaration();

/*
 * local-variable-definition = local-variable-declaration ";"
 */
static AstPtr
parseLocalVariableDefinition()
{
    auto def = parseLocalVariableDeclaration();
    if (!def) {
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return def;
}

//------------------------------------------------------------------------------
/*
 * local-variable-declaration = "local" variable-declaration-list
 */
static AstPtr
parseLocalVariableDeclaration()
{
    if (token.kind != TokenKind::LOCAL) {
	return nullptr;
    }
    getToken();
    auto def = parseVariableDeclarationList();
    if (!def) {
	error::out() << token.loc
	    << ": expected local variable declaration list" << std::endl;
	error::fatal();
	return nullptr;
    }
    return std::make_unique<AstLocalVar>(std::move(def));
}


//------------------------------------------------------------------------------
static AstPtr parseCompoundStatement();
static AstPtr parseIfStatement();
static AstPtr parseReturnStatement();
static AstPtr parseExpressionStatement();

/*
 * statement = compound-statement
 *	     | if-statement
 *	     | switch-statement
 *	     | while-statement
 *	     | do-while-statement
 *	     | for-statement
 *	     | return-statement
 *	     | break-statement
 *	     | continue-statement
 *	     | expression-statement
 *	     | goto-statement
 *	     | label-definition
 */
static AstPtr
parseStatement()
{
    AstPtr ast;

    (ast = parseCompoundStatement())
    || (ast = parseIfStatement())
    || (ast = parseReturnStatement())
    || (ast = parseExpressionStatement());

    return ast;
}
//------------------------------------------------------------------------------
/*
 * compound-statement = "{" statement-or-declaration-list "}"
 */
static AstPtr
parseCompoundStatement()
{
    if (token.kind != TokenKind::LBRACE) {
	return nullptr;
    }
    getToken();

    Symtab newScope;
    auto body = parseStatementOrDeclarationList();

    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return body;
}

//------------------------------------------------------------------------------
/*
 * if-statement = "if" "(" expression ")" compound-statement
 *		    [ "else" if-statement | compound-statement ]
 */
static AstPtr
parseIfStatement()
{
    if (token.kind != TokenKind::IF) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto ifCond = parseExpression();
    if (!ifCond) {
	error::out() << token.loc << ": expression expected" << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto thenBody = parseCompoundStatement();
    if (!thenBody) {
	error::out() << token.loc << ": compound statement expected"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    if (token.kind == TokenKind::ELSE) {
	getToken();
	auto elseBody = parseCompoundStatement();
	if (!elseBody) {
	    elseBody = parseIfStatement();
	}
	if (!elseBody) {
	    error::out() << token.loc << ": compound statement expected"
		<< std::endl;
	    error::fatal();
	    return nullptr;
	}
	return std::make_unique<AstIf>(std::move(ifCond), std::move(thenBody),
				       std::move(elseBody));
    }
    return std::make_unique<AstIf>(std::move(ifCond), std::move(thenBody));
}

//------------------------------------------------------------------------------
/*
 * return-statement = "return" [ expression ] ";"
 */
static AstPtr
parseReturnStatement()
{
    if (token.kind != TokenKind::RETURN) {
	return nullptr;
    }
    getToken();
    auto expr = parseExpression();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstReturn>(std::move(expr));
}

//------------------------------------------------------------------------------
/*
 * expression-statement = [expression] ";"
 */
static AstPtr
parseExpressionStatement()
{
    auto expr = parseExpression();
    if (expr) {
	error::expected(TokenKind::SEMICOLON);
    }
    if (token.kind != TokenKind::SEMICOLON) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstExpr>(std::move(expr));
}

//------------------------------------------------------------------------------
static bool parseEnumConstantDeclaration(AstEnumDeclPtr &enumDecl);

/*
 * enum-declaration
 *	= "enum" identifier [":" integer-type]
 *	    (";" | enum-constant-declaration )
 */
static AstPtr
parseEnumDeclaration()
{
    if (token.kind != TokenKind::ENUM) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return nullptr;
    }
    auto ident = token;
    getToken();
    auto type = IntegerType::createInt();
    if (token.kind == TokenKind::COLON) {
	getToken();
	type = parseType();
	if (!type) {
	    error::out() << token.loc << ": integer type expected" << std::endl;
	    error::fatal();
	    return nullptr;
	}
    }
    auto enumDecl = std::make_unique<AstEnumDecl>(ident, type);

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
    } else if (parseEnumConstantDeclaration(enumDecl)) {
	enumDecl->complete();
    } else {
	error::out() << token.loc
	    << ": ';' or declaration of enum constants expected" << std::endl;
	error::fatal();
	return nullptr;
    }
    return enumDecl;
}
 
//------------------------------------------------------------------------------
static bool parseEnumConstantList(AstEnumDeclPtr &enumDecl);

/*
 * enum-constant-declaration = "{" enum-constant-list "}" ";"
 */
static bool
parseEnumConstantDeclaration(AstEnumDeclPtr &enumDecl)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();
    if (!parseEnumConstantList(enumDecl)) {
	error::out() << token.loc
	    << ": list of enum constants expected" << std::endl;
	error::fatal();
	return false;
    }
    if (!error::expected(TokenKind::RBRACE)) {
	return false;
    }
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return false;
    }
    getToken();
    return true;
}

//------------------------------------------------------------------------------
/*
 * enum-constant-list = identifier [ "=" expression] ["," [enum-constant-list]] 
 *
 */
static bool
parseEnumConstantList(AstEnumDeclPtr &enumDecl)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    while (token.kind == TokenKind::IDENTIFIER) {
	auto ident = token;
	getToken();
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    auto expr = parseExpression();
	    if (!expr) {
		error::out() << token.loc << ": expression expected"
		    << std::endl;
		error::fatal();
		return false;
	    }
	    enumDecl->add(ident, std::move(expr));
	} else {
	    enumDecl->add(ident);
	}
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return true;
}

//------------------------------------------------------------------------------

} // namespace abc
