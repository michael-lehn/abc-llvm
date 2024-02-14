#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>

#include "ast.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "initializerlist.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "ustr.hpp"

//------------------------------------------------------------------------------
static AstPtr parseTopLevelDeclaration();

/*
 * input-sequence = {top-level-declaration} EOI
 */
AstPtr
parser()
{
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
static bool parseTypeDeclaration();
static bool parseStructDeclaration();
static bool parseEnumDeclaration();

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

    if (auto ast = parseFunctionDeclarationOrDefinition()) {
	return ast;
    } else if (auto ast = parseExternDeclaration()) {
	return ast;
    } else if (auto ast = parseGlobalVariableDefinition()) {
	return ast;
    } else if (parseTypeDeclaration()) {
    } else if (parseStructDeclaration()) {
    } else if (parseEnumDeclaration()) {
    } else {
	return nullptr;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
static const Type *parseFunctionType(Token *fnIdent = nullptr,
				     std::vector<Token> *paramIdent = nullptr);
static AstPtr parseCompoundStatement(bool openScope = true);

/*
 * function-declaration-or-definition
 *	= function-type (";" | compound-statement)
 */
static AstPtr
parseFunctionDeclarationOrDefinition()
{
    Token fnIdent;
    std::vector<Token> fnParam;

    const Type *fnType = parseFunctionType(&fnIdent, &fnParam);
    if (!fnType) {
	return nullptr;
    }

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	return std::make_unique<AstFuncDecl>(fnIdent, fnType, fnParam);
    }

    auto fnDef = std::make_unique<AstFuncDef>(fnIdent, fnType, fnParam);
    auto fnBody = parseCompoundStatement(false);

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
static bool parseFunctionParameterList(std::vector<const Type *> &type,
				       bool &hasVarg,
				       std::vector<Token> *ident);

/*
 * function-type
 *	= "fn" [identifier] "(" function-parameter-list ")" [ ":" type ]
 */
static const Type *
parseFunctionType(Token *fnIdent, std::vector<Token> *paramIdent)
{
    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();
    if (token.kind == TokenKind::IDENTIFIER) {
	if (fnIdent) {
	    *fnIdent = token;
	}
	getToken();
    }
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();

    std::vector<const Type *> paramType;
    bool hasVarg;
    if (!parseFunctionParameterList(paramType, hasVarg, paramIdent)) {
	error::out() << token.loc << ": expected parameter list" << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto retType = Type::getVoid();
    if (token.kind == TokenKind::COLON) {
	getToken();
	retType = parseType();
	if (!retType) {
	    error::out() << token.loc << ": expected return type" << std::endl;
	    error::fatal();
	    return nullptr;
	}
    }
    return Type::getFunction(retType, paramType, hasVarg);
}

//------------------------------------------------------------------------------
/*
 * function-parameter-list
 *	= [ [identifier] ":" type { "," [identifier] ":" type} } ["," ...] ]
 */
static bool
parseFunctionParameterList(std::vector<const Type *> &type,
			   bool &hasVarg,
			   std::vector<Token> *ident)
{
    if (token.kind != TokenKind::IDENTIFIER && token.kind != TokenKind::COLON) {
	return true; // empty parameter list
    }
    hasVarg = false;
    while (true) {
	if (token.kind == TokenKind::IDENTIFIER) {
	    if (ident) {
		ident->push_back(token);
	    }
	    getToken();
	} else if (ident) {
	    ident->push_back(Token{});
	}
	if (!error::expected(TokenKind::COLON)) {
	    return false;
	}
	getToken();
	auto ty = parseType();
	if (!ty) {
	    error::out() << token.loc
		<< ": expected argument type" << std::endl;
	    error::fatal();
	    return false;
	}
	type.push_back(ty);
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
static bool parseExternVariableDeclaration();

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
    std::vector<Token> fnParam;
    const Type *fnType = parseFunctionDeclaration(fnIdent, fnParam);

    if (!fnType && !parseExternVariableDeclaration()) {
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    if (fnType) {
	return std::make_unique<AstFuncDecl>(fnIdent, fnType, fnParam, true);
    }
    std::cerr << "parseExternVariableDeclaration() is a TODO\n";
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
    return parseFunctionType(&fnIdent, &param);
}

//------------------------------------------------------------------------------
/*
 * extern-variable-declaration = identifier ":" type
 */
static bool
parseExternVariableDeclaration()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return false;
    }
    getToken();
    if (!parseType()) {
	error::out() << token.loc << ": expected variable type"
	    << std::endl;
	error::fatal();
	return false;
    }
    return true;
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
static bool parseInitializer(InitializerList &initList);

/*
 * variable-declaration = identifier ":" type [ "=" initializer ]
 */
static AstVarPtr
parseVariableDeclaration()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return nullptr;
    }
    auto loc = token.loc;
    auto ident = token.val;
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return nullptr;
    }
    getToken();
    auto type = parseType();
    if (!type) {
	error::out() << token.loc << ": expected variable type" << std::endl;
	error::fatal();
	return nullptr;
    }
    InitializerList init(type);
    if (token.kind == TokenKind::EQUAL) {
	getToken();
	if (!parseInitializer(init)) {
	    error::out() << token.loc << ": initializer expected" << std::endl;
	    error::fatal();
	    return nullptr;
	}
    }
    return std::make_unique<AstVar>(ident, type, loc, std::move(init));
}

//------------------------------------------------------------------------------
//static bool parseStringLiteral();

/*
 * initializer = expression
 *             | string-literal
 *             | initializer-list
 */
static bool
parseInitializer(InitializerList &init)
{
    if (auto expr = parseExpression()) {
	init.add(std::move(expr));
	return true;
    } else if (parseInitializerList(init)) {
	return true;
    }
    /*
    return parseExpression()
	|| parseStringLiteral()
	|| parseInitializerList(initList);
    */
    return false;
}

//------------------------------------------------------------------------------
/*
static bool
parseStringLiteral()
{
    if (token.kind != TokenKind::STRING_LITERAL) {
	return false;
    }
    getToken();
    return true;
}
*/

//------------------------------------------------------------------------------
/*
 * initializer-list = "{" initializer-sequence "}"
 * initializer-sequence = [ initializer ["," [initializer-sequence] ] 
 */
bool
parseInitializerList(InitializerList &initList)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();
    // parseInitializerSequence:
    while (parseInitializer(initList)) {
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    if (!error::expected(TokenKind::RBRACE)) {
	return false;
    }
    getToken();
    return true;
}

//------------------------------------------------------------------------------
/*
 * type-declaration = "type" identifier ":" type ";"
 */
static bool
parseTypeDeclaration()
{
    if (token.kind != TokenKind::TYPE) {
	return false;
    }
    getToken();
    if (token.kind != TokenKind::IDENTIFIER) {
	error::out() << token.loc << ": identifier for type expected"
	    << std::endl;
	error::fatal();
	return false;
    }
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return false;
    }
    if (!parseType()) {
	error::out() << token.loc << ": type expected"
	    << std::endl;
	error::fatal();
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return false;
    }
    return true;
}

//------------------------------------------------------------------------------
static bool parseStructMemberDeclaration();

/*
 * struct-declaration = "struct" identifier (";" | struct-member-declaration )
 */
static bool
parseStructDeclaration()
{
    if (token.kind != TokenKind::STRUCT) {
	return false;
    }
    getToken();
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return false;
    }
    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
    } else if (parseStructMemberDeclaration()) {
    } else {
	error::out() << token.loc
	    << ": ';' or struct member declaration expected" << std::endl;
	error::fatal();
	return false;
    }
    return true;
}

//------------------------------------------------------------------------------
static bool parseStructMemberList();

/*
 * struct-member-declaration = "{" { struct-member-list } "}" ";"
 */
static bool
parseStructMemberDeclaration()
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();
    while (parseStructMemberList()) {
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
 * struct-member-list
 *	= identifier { "," identifier } ":" ( type | struct-declaration ) ";"
 */
static bool
parseStructMemberList()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    getToken();
    while (token.kind == TokenKind::COMMA) {
	getToken();
	if (token.kind != TokenKind::IDENTIFIER) {
	    error::out() << token.loc
		<< ": identifier for struct member expected" << std::endl;
	    error::fatal();
	    return false;
	}
	getToken();
    }
    if (!error::expected(TokenKind::COLON)) {
	return false;
    }
    getToken();
    if (parseType()) {
    } else if (parseStructDeclaration()) {
    } else {
	error::out() << token.loc << ": expected type or struct declaration"
	    << std::endl;
	error::fatal();
	return false;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return false;
    }
    getToken();
    return true;
}

//------------------------------------------------------------------------------
static bool parseEnumConstantDeclaration();

/*
 * enum-declaration
 *	= "enum" identifier [":" integer-type]
 *	    (";" | enum-constant-declaration )
 */
static bool
parseEnumDeclaration()
{
    if (token.kind != TokenKind::ENUM) {
	return false;
    }
    getToken();
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return false;
    }
    getToken();
    if (token.kind == TokenKind::COLON) {
	getToken();
	if (!parseType()) {
	    error::out() << token.loc << ": integer type expected" << std::endl;
	    error::fatal();
	    return false;
	}
    }
    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
    } else if (parseEnumConstantDeclaration()) {
    } else {
	error::out() << token.loc
	    << ": ';' or declaration of enum constants expected" << std::endl;
	error::fatal();
	return false;
    }
    return true;
}
 
//------------------------------------------------------------------------------
static bool parseEnumConstantList();

/*
 * enum-constant-declaration = "{" enum-constant-list "}" ";"
 */
static bool
parseEnumConstantDeclaration()
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();
    if (!parseEnumConstantList()) {
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
parseEnumConstantList()
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    while (token.kind == TokenKind::IDENTIFIER) {
	getToken();
	if (token.kind == TokenKind::EQUAL) {
	    getToken();
	    if (!parseExpression()) {
		error::out() << token.loc << ": expression expected"
		    << std::endl;
		error::fatal();
		return false;
	    }
	}
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return true;
}

//------------------------------------------------------------------------------
static AstPtr parseStatement();
static AstPtr parseDeclaration();

/*
 * compound-statement = "{" { statement | declaration } "}"
 */
static AstPtr
parseCompoundStatement(bool newScope)
{
    if (token.kind != TokenKind::LBRACE) {
	return nullptr;
    }
    getToken();

    if (newScope) {
	Symtab::openScope();
    }
    
    auto compound = std::make_unique<AstCompound>();
    for (AstPtr ast; (ast = parseStatement()) || (ast = parseDeclaration());) {
	compound->append(std::move(ast));
    }

    if (newScope) {
	Symtab::closeScope();
    }

    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return compound;
}

//------------------------------------------------------------------------------
static AstPtr parseIfStatement();
static AstPtr parseSwitchStatement();
static AstPtr parseWhileStatement();
static AstPtr parseForStatement();
static AstPtr parseReturnStatement();
static AstPtr parseBreakStatement();
static AstPtr parseContinueStatement();
static AstPtr parseExpressionStatement();

/*
 * statement = compound-statement
 *	     | if-statement
 *	     | switch-statement
 *	     | while-statement
 *	     | for-statement
 *	     | return-statement
 *	     | break-statement
 *	     | continue-statement
 *	     | expression-statement
 */
static AstPtr
parseStatement()
{
    AstPtr ast;

    (ast = parseCompoundStatement())
    || (ast = parseIfStatement())
    || (ast = parseSwitchStatement())
    || (ast = parseWhileStatement())
    || (ast = parseForStatement())
    || (ast = parseReturnStatement())
    || (ast = parseBreakStatement())
    || (ast = parseContinueStatement())
    || (ast = parseExpressionStatement());
    return ast;
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

    if (parseTypeDeclaration()
	|| parseEnumDeclaration()
	|| parseStructDeclaration()
	) {
	std::cerr << "no ast generated\n";
    }

    (ast = parseGlobalVariableDefinition())
	|| (ast = parseLocalVariableDefinition());
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
/*
 * if-statement = "if" "(" expression ")" compound-statement
 *		    [ "else" compound-statement ]
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
 * switch-statement = "switch" "(" expression ")"
 *			"{" switch-case-or-statement "}"
 * switch-case-or-statement = "case" expression ":"
 *			    | "default" ":" 
 *			    | statement
 */
static AstPtr
parseSwitchStatement()
{
    if (token.kind != TokenKind::SWITCH) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto switchExpr = parseExpression();
    if (!switchExpr) {
	error::out() << token.loc << ": expression expected" << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LBRACE)) {
	return nullptr;
    }
    getToken();

    auto switchStmt = std::make_unique<AstSwitch>(std::move(switchExpr));

    // { parseSwitchCaseOrStatement }:
    while (true) {
	if (token.kind == TokenKind::CASE) {
	    getToken();
	    auto expr = parseExpression();
	    if (!expr) {
		error::out() << token.loc << ": expression expected"
		    << std::endl;
		error::fatal();
	    }
	    if (!error::expected(TokenKind::COLON)) {
		return nullptr;
	    }
	    getToken();
	    switchStmt->appendCase(std::move(expr));
	} else if (token.kind == TokenKind::DEFAULT) {
	    getToken();
	    if (!error::expected(TokenKind::COLON)) {
		return nullptr;
	    }
	    getToken();
	    switchStmt->appendDefault();
	} else if (auto ast = parseStatement()) {
	    switchStmt->append(std::move(ast));
	} else {
	    break;
	}
    }

    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return switchStmt;
}

/*
 * while-statement = "while" "(" expression ")" compound-statement
 */
static AstPtr
parseWhileStatement()
{
    if (token.kind != TokenKind::WHILE) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto whileCond = parseExpression();
    if (!whileCond) {
	error::out() << token.loc << ": expression expected" << std::endl;
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto whileBody = parseCompoundStatement();
    if (!whileBody) {
	error::out() << token.loc << ": compound statement expected"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    return std::make_unique<AstWhile>(std::move(whileCond),
				      std::move(whileBody));
}

/*
 * for-statement = "for" "(" [expression-or-local-variable-definition] ";"
 *			[expression] ";" [expression] ")"
 *			compound-statement
 *  expression-or-local-variable-definition = expression
 *					    | local-variable-declaration
 */
static AstPtr
parseForStatement()
{
    if (token.kind != TokenKind::FOR) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto forInit = parseExpression();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    auto forCond = parseExpression();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    auto forUpdate = parseExpression();
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto forLoop = std::make_unique<AstFor>(std::move(forInit),
					    std::move(forCond),
					    std::move(forUpdate));
    auto forBody = parseCompoundStatement(false);
    if (!forBody) {
	error::out() << token.loc << ": compound statement expected"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    forLoop->appendBody(std::move(forBody));
    return forLoop;
}

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

/*
 * break-statement = "break" ";"
 */
static AstPtr
parseBreakStatement()
{
    if (token.kind != TokenKind::BREAK) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstBreak>();
}

/*
 * continue-statement = "continue" ";"
 */
static AstPtr
parseContinueStatement()
{
    if (token.kind != TokenKind::CONTINUE) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstContinue>();
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
	return Type::getConst(type);
    } else if (auto type = parseUnqualifiedType()) {
	return type;
    } else {
	return nullptr;
    }
}

//------------------------------------------------------------------------------
static const Type *parsePointerType();
static const Type *parseArrayType();

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
	auto type = Symtab::getNamedType(token.val, Symtab::AnyScope);
	getToken();
	return type;
    } else if (auto type = parsePointerType()) {
	return type;
    } else if (auto type = parseArrayType()) {
	return type;
    } else if (auto type = parseFunctionType()) {
	return type;
    } else {
	return nullptr;
    }
}

//------------------------------------------------------------------------------
/*
 * pointer-type = "->" type
 */
static const Type *
parsePointerType()
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto type = parseType();
    if (!type) {
	error::out() << token.loc << ": type expected"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    return Type::getPointer(type);
}

//------------------------------------------------------------------------------
/*
 * array-type = "array" "[" expression "]" { "[" expression "]" } "of" type
 */
static const Type *
parseArrayType()
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    do {
	if (!error::expected(TokenKind::LBRACKET)) {
	    return nullptr;
	}
	getToken();
	if (!parseExpression()) {
	    error::out() << token.loc << ": constant expression expected"
		<< std::endl;
	    error::fatal();
	    return nullptr;
	}
	if (!error::expected(TokenKind::RBRACKET)) {
	    return nullptr;
	}
	getToken();
    } while (token.kind == TokenKind::LBRACKET);
    if (!error::expected(TokenKind::OF)) {
	return nullptr;
    }
    getToken();
    if (!parseType()) {
	error::out() << token.loc << ": element type expected"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    std::cerr << "parseArrayType() always returns nullptr\n";
    return nullptr;
}
