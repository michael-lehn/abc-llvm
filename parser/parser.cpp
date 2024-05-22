#include <iostream>
#include <sstream>

#include "expr/expr.hpp"
#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"
#include "type/arraytype.hpp"
#include "type/functiontype.hpp"
#include "type/integertype.hpp"
#include "type/pointertype.hpp"
#include "type/voidtype.hpp"

#include "defaultdecl.hpp"
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
    initDefaultDecl();
    getToken();

    auto top = std::make_unique<AstList>();
    while (auto decl = parseTopLevelDeclaration()) {
	top->append(std::move(decl));
    }
    if (token.kind != TokenKind::EOI) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "unexpected "
	    << token.val.c_str() << "\n"
	    << error::setColor(error::NORMAL);
	return nullptr;
    }
    getToken();
    return top;
}

//------------------------------------------------------------------------------
static AstPtr parseFunctionDeclarationOrDefinition();
static AstPtr parseExternDeclaration();
static AstPtr parseGlobalVariableDefinition();
static AstPtr parseTypeDeclaration();
static AstPtr parseStructDeclaration();
static AstPtr parseEnumDeclaration();

/*
 * top-level-declaration = function-declaration-or-definition
 *			 | extern-declaration
 *			 | global-variable-definition
 *			 | type-declaration
 *			 | enum-declaration
 *			 | struct-declaration
 */
static AstPtr
parseTopLevelDeclaration()
{
    AstPtr ast;

    (ast = parseFunctionDeclarationOrDefinition())
    || (ast = parseExternDeclaration())
    || (ast = parseGlobalVariableDefinition())
    || (ast = parseTypeDeclaration())
    || (ast = parseEnumDeclaration())
    || (ast = parseStructDeclaration());

    return ast;
}

//------------------------------------------------------------------------------
static const Type *parseFunctionType(Token &fnName,
				     std::vector<Token> &fnParamName);
static AstPtr parseBlock();

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

    auto fnBody = parseBlock();
    if (!fnBody) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected ';' or compound statement\n"
	    << error::setColor(error::NORMAL);
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
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": expected parameter list\n"
	    << error::setColor(error::NORMAL);
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
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "expected return type\n"
		<< error::setColor(error::NORMAL);
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
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "expected parameter type\n"
		<< error::setColor(error::NORMAL);
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
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected extern function or variable declaration\n"
	    << error::setColor(error::NORMAL);
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
 * block = "{" statement-or-declaration-list "}"
 */
static AstPtr
parseBlock()
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
static const Type *parseUnqualifiedType(bool allowZeroDim);

/*
 * type = [const] unqualified-type
 */
const Type *
parseType(bool allowZeroDim)
{
    if (token.kind == TokenKind::CONST) {
	getToken();
	auto type = parseUnqualifiedType(allowZeroDim);
	if (!type) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "unqualified type expected\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	return type->getConst();
    } else if (auto type = parseUnqualifiedType(allowZeroDim)) {
	return type;
    } else {
	return nullptr;
    }
}

//------------------------------------------------------------------------------
static const Type *parsePointerType();
static const Type *parseArrayType(bool allowZeroDim);

/*
 * unqualified-type = identifier
 *		    | pointer-type
 *		    | array-type
 *		    | function-type
 */
static const Type *
parseUnqualifiedType(bool allowZeroDim)
{
    Token fnName;
    std::vector<Token> fnParamName;

    if (token.kind == TokenKind::IDENTIFIER) {
	if (auto entry = Symtab::type(token.val, Symtab::AnyScope)) {
	    getToken();
	    return entry->type;
	}
	return nullptr;
    } else if (auto type = parsePointerType()) {
	return type;
    } else if (auto type = parseArrayType(allowZeroDim)) {
	return type;
    } else if (auto type = parseFunctionType(fnName, fnParamName)) {
	return type;
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
static bool parseIdentifierList(std::vector<Token> &identifier);

/*
 * extern-variable-declaration = identifier-list ":" type
 *				 { "," identifier-list ":" type }
 */
static AstListPtr
parseExternVariableDeclaration()
{
    auto astList = std::make_unique<AstList>();

    while (true) {
	std::vector<Token> varName;
	if (!parseIdentifierList(varName)) {
	    return nullptr;
	}
	if (!error::expected(TokenKind::COLON)) {
	    return nullptr;
	}
	getToken();
	auto varTypeLoc = token.loc;
	auto varType = parseType(true);
	if (!varType) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "expected variable type\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	astList->append(std::make_unique<AstVar>(std::move(varName),
						 varTypeLoc,
						 varType,
						 false));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return astList;
}

//------------------------------------------------------------------------------
/*
 * identifier-list = identifier { "," identifier }
 */
static bool
parseIdentifierList(std::vector<Token> &identifier)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    identifier.clear();
    while (true) {
	error::expected(TokenKind::IDENTIFIER);
	identifier.push_back(token);
	getToken();
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return true;
}

//------------------------------------------------------------------------------
static AstListPtr parseVariableDefinitionList();

/*
 * global-variable-definition = "global" variable-definition-list
 */
static AstPtr
parseGlobalVariableDefinition()
{
    if (token.kind != TokenKind::GLOBAL) {
	return nullptr;
    }
    getToken();
    auto def = parseVariableDefinitionList();
    if (!def) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected global variable definition list\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstGlobalVar>(std::move(def));
}

//------------------------------------------------------------------------------

/*
 * global-variable-definition = "global" variable-definition-list
 */
static AstPtr
parseStaticVariableDefinition()
{
    if (token.kind != TokenKind::STATIC) {
	return nullptr;
    }
    getToken();
    auto def = parseVariableDefinitionList();
    if (!def) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected static variable definition list\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstStaticVar>(std::move(def));
}

//------------------------------------------------------------------------------
static AstVarPtr parseVariableDefinition();

/*
 * variable-definition-list = variable-definition { "," variable-definition }
 */
static AstListPtr
parseVariableDefinitionList()
{
    auto astList = std::make_unique<AstList>();

    for (bool first = true; ; first = false) {
	auto def = parseVariableDefinition();
	if (!def) {
	    if (!first) {
		error::location(token.loc);
		error::out() << error::setColor(error::BOLD) << token.loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": expected variable declaration\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
	    }
	    return nullptr;
	}
	astList->append(std::move(def));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    return astList;
}

//------------------------------------------------------------------------------
static AstInitializerExprPtr parseInitializerExpression(const Type *type);

/*
 * variable-definition = identifier-list ":" type
 *			  [ "=" initializer-expression ]
 */
static AstVarPtr
parseVariableDefinition()
{
    std::vector<Token> varName;
    if (!parseIdentifierList(varName)) {
	return nullptr;
    }
    if (!error::expected(TokenKind::COLON)) {
	return nullptr;
    }
    getToken();
    auto varTypeLoc = token.loc;
    auto varType = parseType(true);
    if (!varType) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected variable type\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    auto astVar = std::make_unique<AstVar>(std::move(varName),
					   varTypeLoc,
					   varType,
					   true);
    if (varType->isUnboundArray() && token.kind != TokenKind::EQUAL) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "unbound array requires an initializer list\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (token.kind == TokenKind::EQUAL) {
	getToken();
	auto initializerType = astVar->varName.size() > 1
	    ? ArrayType::create(varType, astVar->varName.size())
	    : varType;
	auto initializer = parseInitializerExpression(initializerType);
	if (!initializer) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< (initializerType->isScalar()
			? "expression of initializer list expected\n"
			: "initializer list expected\n")
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	astVar->addInitializerExpr(std::move(initializer));
    }
    return astVar;
}

//------------------------------------------------------------------------------
/*
 * initializer-expression = expression
 *			  | initializer-list   ?? own rule ??
 */
static AstInitializerExprPtr
parseInitializerExpression(const Type *type)
{
    // note: parseCompoundExpression has to be called before
    //	     parseAssignmentExpression. Because parseCompoundExpression catches
    //	     string literals and treats them as a compound.
    if (auto expr = parseCompoundExpression(type, &type)) {
	return std::make_unique<AstInitializerExpr>(type, std::move(expr));
    } else if (auto expr = parseAssignmentExpression()) {
	return std::make_unique<AstInitializerExpr>(type, std::move(expr));
    }
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

    (ast = parseTypeDeclaration())
	|| (ast = parseEnumDeclaration())
	|| (ast = parseStructDeclaration())
	|| (ast = parseStaticVariableDefinition())
	|| (ast = parseLocalVariableDefinition());
    return ast;
}

//------------------------------------------------------------------------------
/*
 * local-variable-definition = "local" variable-definition-list
 */
static AstPtr
parseLocalVariableDefinition()
{
    if (token.kind != TokenKind::LOCAL) {
	return nullptr;
    }
    getToken();
    auto def = parseVariableDefinitionList();
    if (!def) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected local variable declaration list\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstLocalVar>(std::move(def));
}


//------------------------------------------------------------------------------
static AstPtr parseCompoundStatement();
static AstPtr parseIfStatement();
static AstPtr parseSwitchStatement();
static AstPtr parseWhileStatement();
static AstPtr parseDoWhileStatement();
static AstPtr parseForStatement();
static AstPtr parseReturnStatement();
static AstPtr parseBreakStatement();
static AstPtr parseContinueStatement();
static AstPtr parseGotoStatement();
static AstPtr parseLabelDefinition();
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
 *	     | goto-statement
 *	     | label-definition
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
    || (ast = parseDoWhileStatement())
    || (ast = parseForStatement())
    || (ast = parseReturnStatement())
    || (ast = parseBreakStatement())
    || (ast = parseContinueStatement())
    || (ast = parseGotoStatement())
    || (ast = parseLabelDefinition())
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
    auto tok = token;
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto ifCond = parseExpressionList();
    if (!ifCond) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": expression expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto thenBody = parseCompoundStatement();
    if (!thenBody) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": compound statement expected\n"
	    << error::setColor(error::NORMAL);
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
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "compound statement expected\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	return std::make_unique<AstIf>(tok.loc, std::move(ifCond),
				       std::move(thenBody),
				       std::move(elseBody));
    }
    return std::make_unique<AstIf>(tok.loc, std::move(ifCond),
				   std::move(thenBody));
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
    auto switchExpr = parseExpressionList();
    if (!switchExpr) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expression expected\n"
	    << error::setColor(error::NORMAL);
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
	    auto expr = parseExpressionList();
	    if (!expr) {
		error::location(token.loc);
		error::out() << error::setColor(error::BOLD) << token.loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "expression expected\n"
		    << error::setColor(error::NORMAL);
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
    switchStmt->complete();

    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return switchStmt;
}
//------------------------------------------------------------------------------
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
    auto whileCond = parseExpressionList();
    if (!whileCond) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": expression expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto whileBody = parseCompoundStatement();
    if (!whileBody) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": compound statement expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    return std::make_unique<AstWhile>(std::move(whileCond),
				      std::move(whileBody));
}

//------------------------------------------------------------------------------
static AstPtr
parseDoWhileStatement()
{
    if (token.kind != TokenKind::DO) {
	return nullptr;
    }
    getToken();
    auto doWhileBody = parseCompoundStatement();
    if (!doWhileBody) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": compound statement expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::WHILE)) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::LPAREN)) {
	return nullptr;
    }
    getToken();
    auto doWhileCond = parseExpressionList();
    if (!doWhileCond) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": expression expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstDoWhile>(std::move(doWhileCond),
				        std::move(doWhileBody));
}

//------------------------------------------------------------------------------
/*
 * for-statement = "for" "(" [expression-or-variable-definition] ";"
 *			[expression] ";" [expression] ")"
 *			compound-statement
 *  expression-or-variable-definition = expression
 *				      | local-variable-declaration
 *				      | global-variable-declaration
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

    Symtab newScope;
    AstPtr forInitDecl;
    ExprPtr forInitExpr;
    if (!(forInitDecl = parseLocalVariableDefinition())) {
	if (!(forInitDecl = parseGlobalVariableDefinition())) {
	    forInitExpr = parseExpressionList();
	    if (!error::expected(TokenKind::SEMICOLON)) {
		return nullptr;
	    }
	    getToken();
	}
    }
    auto forCond = parseExpressionList();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    auto forUpdate = parseExpressionList();
    if (!error::expected(TokenKind::RPAREN)) {
	return nullptr;
    }
    getToken();
    auto forLoop = forInitDecl
	?  std::make_unique<AstFor>(std::move(forInitDecl),
				    std::move(forCond),
				    std::move(forUpdate))
	: std::make_unique<AstFor>(std::move(forInitExpr),
				   std::move(forCond),
				   std::move(forUpdate));
    auto forBody = parseBlock();
    if (!forBody) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": compound statement expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    forLoop->appendBody(std::move(forBody));
    return forLoop;
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
    auto tok = token;
    getToken();
    auto expr = parseExpressionList();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstReturn>(tok.loc, std::move(expr));
}

//------------------------------------------------------------------------------
/*
 * break-statement = "break" ";"
 */
static AstPtr
parseBreakStatement()
{
    if (token.kind != TokenKind::BREAK) {
	return nullptr;
    }
    auto loc = token.loc;
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstBreak>(loc);
}
//------------------------------------------------------------------------------
/*
 * continue-statement = "continue" ";"
 */
static AstPtr
parseContinueStatement()
{
    if (token.kind != TokenKind::CONTINUE) {
	return nullptr;
    }
    auto loc = token.loc;
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstContinue>(loc);
}

//------------------------------------------------------------------------------

static AstPtr
parseGotoStatement()
{
    if (token.kind != TokenKind::GOTO) {
	return nullptr;
    }
    getToken();
    auto label = token;
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return nullptr;
    }
    getToken();

    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstGoto>(label.loc, label.val);
}

//------------------------------------------------------------------------------

static AstPtr
parseLabelDefinition()
{
    if (token.kind != TokenKind::LABEL) {
	return nullptr;
    }
    getToken();
    auto label = token;
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstLabel>(label.loc, label.val);
}

//------------------------------------------------------------------------------
/*
 * expression-statement = [expression] ";"
 */
static AstPtr
parseExpressionStatement()
{
    auto expr = parseExpressionList();
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
/*
 * type-declaration = "type" identifier ":" type ";"
 */
static AstPtr
parseTypeDeclaration()
{
    if (token.kind != TokenKind::TYPE) {
	return nullptr;
    }
    getToken();
    if (token.kind != TokenKind::IDENTIFIER) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "identifier for type expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    auto ident = token;
    getToken();
    if (!error::expected(TokenKind::COLON)) {
	return nullptr;
    }
    getToken();
    auto type = parseType();
    if (!type) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "type expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstTypeDecl>(ident, type);
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
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "integer type expected\n"
		<< error::setColor(error::NORMAL);
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
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "';' or declaration of enum constants expected\n"
	    << error::setColor(error::NORMAL);
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
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "list of enum constants expected\n"
	    << error::setColor(error::NORMAL);
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
	    auto expr = parseAssignmentExpression();
	    if (!expr) {
		error::location(token.loc);
		error::out() << error::setColor(error::BOLD) << token.loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "expression expected\n"
		    << error::setColor(error::NORMAL);
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
static bool parseStructMemberDeclaration(AstStructDecl *structDecl);

/*
 * struct-declaration = "struct" identifier (";" | struct-member-declaration )
 */
static AstPtr
parseStructDeclaration()
{
    if (token.kind != TokenKind::STRUCT) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return nullptr;
    }
    auto structTypeName = token;
    getToken();

    auto structDecl = std::make_unique<AstStructDecl>(structTypeName);

    if (token.kind == TokenKind::SEMICOLON) {
        getToken();
    } else if (parseStructMemberDeclaration(structDecl.get())) {
        structDecl->complete();
    } else {
        error::location(token.loc);
        error::out() << error::setColor(error::BOLD) << token.loc << ": "
            << error::setColor(error::BOLD_RED) << "error: "
            << error::setColor(error::BOLD)
            << "';' or struct member declaration expected\n"
            << error::setColor(error::NORMAL);
        error::fatal();
        structDecl = nullptr;
    }
    return structDecl;
}

//------------------------------------------------------------------------------
static bool parseStructMemberList(AstStructDecl *structDecl,
				  std::size_t &index,
				  bool unionSection);

/*
 * struct-member-declaration = "{" { struct-member-list } "}" ";"
 */
static bool
parseStructMemberDeclaration(AstStructDecl *structDecl)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    bool unionSection = false;
    std::size_t index = 0;
    while (true) {
	if (!unionSection && token.kind == TokenKind::UNION) {
	    getToken();
	    if (!error::expected(TokenKind::LBRACE)) {
		return false;
	    }
	    getToken();
	    unionSection = true;
	}
	if (!parseStructMemberList(structDecl, index, unionSection)) {
	    break;
	}
	if (unionSection && token.kind == TokenKind::RBRACE) {
	    getToken();
	    if (!error::expected(TokenKind::SEMICOLON)) {
		return false;
	    }
	    getToken();
	    unionSection = false;
	    ++index;
	}
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
parseStructMemberList(AstStructDecl *structDecl, std::size_t &index,
		      bool unionSection)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    std::vector<lexer::Token> memberName;
    std::vector<std::size_t> memberIndex;
    
    while (true) {
	if (token.kind != TokenKind::IDENTIFIER) {
	    error::out() << token.loc
		<< ": identifier for struct member expected" << std::endl;
	    error::fatal();
	    return false;
	}
	memberName.push_back(token);
	memberIndex.push_back(index);
	if (!unionSection) {
	    ++index;
	}
	getToken();
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    if (!error::expected(TokenKind::COLON)) {
	return false;
    }
    getToken();
    auto type = parseType();
    if (type) {
	if (!error::expected(TokenKind::SEMICOLON)) {
	    return false;
	}
	getToken();
	structDecl->add(std::move(memberName), std::move(memberIndex), type);
	return true;
    } else if (auto ast = parseStructDeclaration()) {
	structDecl->add(std::move(memberName), std::move(memberIndex),
			std::move(ast));
	return true;
    } else {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected type or struct declaration\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return false;
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
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "type expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    return PointerType::create(type);
}

//------------------------------------------------------------------------------
static const Type * parseArrayDimAndType(bool allowZeroDim = false);

/*
 * array-type = "array" array-dim-and-type
 */
static const Type *
parseArrayType(bool allowZeroDim)
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    auto type = parseArrayDimAndType(allowZeroDim);
    if (!type) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "array type expected\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    return type;
}

//------------------------------------------------------------------------------
/*
 * array-dim-and-type = "[" expression "]" { "[" expression "]" } "of" type
 */
static const Type *
parseArrayDimAndType(bool allowZeroDim)
{
    if (token.kind != TokenKind::LBRACKET) {
	return nullptr;
    }
    getToken();
    ExprPtr dimExpr = nullptr;
    if (!allowZeroDim || token.kind != TokenKind::RBRACKET) {
	dimExpr = parseAssignmentExpression();
	if (!dimExpr || !dimExpr->isConst() || !dimExpr->type->isInteger()) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "constant integer expression expected for array dimension\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
    }
    auto dim = dimExpr ? dimExpr->getSignedIntValue() : 0;
    if (dimExpr && dim <= 0) {
	error::location(token.loc);
	error::out() << error::setColor(error::BOLD) << token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "dimension has to be positiv\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!error::expected(TokenKind::RBRACKET)) {
	return nullptr;
    }
    getToken();
    if (token.kind == TokenKind::OF) {
	getToken();
	auto type = parseType();
	if (!type) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "type expected\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
	return ArrayType::create(type, dim);
    } else {
	auto type = parseArrayDimAndType();
	if (!type) {
	    error::location(token.loc);
	    error::out() << error::setColor(error::BOLD) << token.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "expected 'of' or array dimension\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	return ArrayType::create(type, dim);
    }
}

//------------------------------------------------------------------------------

} // namespace abc
