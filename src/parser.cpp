#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>

#include "ast.hpp"
#include "error.hpp"
#include "initializerlist.hpp"
#include "integerliteral.hpp"
#include "lexer.hpp"
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
static AstPtr parseTypeDeclaration();
static AstStructDeclPtr parseStructDeclaration();
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
    || (ast = parseTypeDeclaration())
    || (ast = parseStructDeclaration())
    || (ast = parseEnumDeclaration());

    return ast;
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
    bool hasVarg = false;
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
    std::vector<Token> fnParam;
    auto fnType = parseFunctionDeclaration(fnIdent, fnParam);
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
	return std::make_unique<AstFuncDecl>(fnIdent, fnType, fnParam, true);
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
    return parseFunctionType(&fnIdent, &param);
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
	auto ident = token;
	getToken();
	if (!error::expected(TokenKind::COLON)) {
	    return nullptr;
	}
	getToken();
	auto type = parseType();
	if (!type) {
	    error::out() << token.loc << ": expected variable type"
		<< std::endl;
	    error::fatal();
	    return nullptr;
	}
	astList->append(std::make_unique<AstVar>(ident.val, type, ident.loc));
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
static AstInitializerListPtr parseInitializer(const Type *type);

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
    auto astVar = std::make_unique<AstVar>(ident, type, loc);
    if (token.kind == TokenKind::EQUAL) {
	getToken();
	auto initializer = parseInitializer(type);
	if (!initializer) {
	    error::out() << token.loc << ": initializer expected" << std::endl;
	    error::fatal();
	    return nullptr;
	}
	astVar->addInitializer(std::move(initializer));
    }
    return astVar;
}

//------------------------------------------------------------------------------
/*
 * initializer = expression
 *             | initializer-list
 */
static AstInitializerListPtr
parseInitializer(const Type *type)
{
    if (!type) {
	return nullptr;
    }
    if (auto list = parseInitializerList(type)) {
	return list;
    } else if (auto expr = parseExpression()) {
	if (Type::getTypeConversion(expr->type, type, expr->loc, true)) {
	    return std::make_unique<AstInitializerList>(type, std::move(expr));
	} else {
	    Type::getTypeConversion(expr->type, type, expr->loc, false);
	    error::out() << expr->loc << ": error: expression of type '"
		<< expr->type << "' can not be used to initialize a variable "
		<< "of type '" << type << "'" << std::endl;
	    error::fatal();
	}
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/*
 * initializer-list = "{" initializer-sequence "}"
 *		    | string-literal
 * initializer-sequence = [ initializer ["," [initializer-sequence] ] 
 */
AstInitializerListPtr
parseInitializerList(const Type *type)
{
    // parse string-literal
    if (type->isArray() && token.kind == TokenKind::STRING_LITERAL) {
	auto loc = token.loc;
	auto str = token.val;
	getToken();
	return std::make_unique<AstInitializerList>(type, loc, str);
    }

    // parse "{" initializer-sequence "}"
    if (token.kind != TokenKind::LBRACE) {
	return nullptr;
    }
    getToken();

    auto list = std::make_unique<AstInitializerList>(type);

    // parseInitializerSequence:
    for (std::size_t index = 0; auto ty = type->getMemberType(index); ++index) {
	auto lbraceRequired = ty->isArray() || ty->isStruct();
	auto item = parseInitializer(type->getMemberType(index));
	if (!item) {
	    if (token.kind != TokenKind::RBRACE && lbraceRequired) {
		if (auto expr = parseExpression()) {
		    error::out() << expr->loc
			<< ": error: use '{ ..}' for initializing elements "
			<< "where type is a struct or an array" << std::endl;
		    error::fatal();
		}
	    }
	    break;
	}
	list->append(std::move(item));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
    }
    if (!error::expected(TokenKind::RBRACE)) {
	return nullptr;
    }
    getToken();
    return list;
}

AstPtr
parseCompoundLiteral(const Type *type)
{
    auto loc = token.loc;
    if (auto list = parseInitializerList(type)) {
	AstListPtr declTmp = std::make_unique<AstList>();
	auto tmp = UStr::create("tmp");
	declTmp->append(std::make_unique<AstVar>(tmp, type, loc));
	return std::make_unique<AstLocalVar>(std::move(declTmp));
    }
    return nullptr;
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
	error::out() << token.loc << ": identifier for type expected"
	    << std::endl;
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
	error::out() << token.loc << ": type expected"
	    << std::endl;
	error::fatal();
    }
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstTypeDecl>(ident, type);
}

//------------------------------------------------------------------------------
static bool parseStructMemberDeclaration(
		    std::vector<Token> &member,
		    std::vector<AstStructDecl::AstOrType> &memberType);

/*
 * struct-declaration = "struct" identifier (";" | struct-member-declaration )
 */
static AstStructDeclPtr
parseStructDeclaration()
{
    if (token.kind != TokenKind::STRUCT) {
	return nullptr;
    }
    getToken();
    if (!error::expected(TokenKind::IDENTIFIER)) {
	return nullptr;
    }
    auto ident = token;
    getToken();

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	return std::make_unique<AstStructDecl>(ident);
    }

    std::vector<Token> member;
    std::vector<AstStructDecl::AstOrType> memberType;

    auto decl = std::make_unique<AstStructDecl>(ident);
    if (parseStructMemberDeclaration(member, memberType)) {
	decl->complete(std::move(member), std::move(memberType));
	return decl;
    }
    error::out() << token.loc
	<< ": ';' or struct member declaration expected" << std::endl;
    error::fatal();
    return nullptr;
}

//------------------------------------------------------------------------------
static bool parseStructMemberList(std::vector<Token> &ident,
				  AstStructDecl::AstOrType &astOrType);

/*
 * struct-member-declaration = "{" { struct-member-list } "}" ";"
 */
static bool
parseStructMemberDeclaration(std::vector<Token> &member,
			     std::vector<AstStructDecl::AstOrType> &memberType)
{
    if (token.kind != TokenKind::LBRACE) {
	return false;
    }
    getToken();

    while (true) {
	std::vector<Token> ident;
	AstStructDecl::AstOrType astOrType;
	if (!parseStructMemberList(ident, astOrType)) {
	    break;
	}
	const Type *type = std::holds_alternative<const Type *>(astOrType)
	    ? std::get<const Type *>(astOrType)
	    : std::get<AstPtr>(astOrType)->getType();
	for (std::size_t i = 0; i < ident.size(); ++i) {
	    member.push_back(ident[i]);
	    i == 0 ? memberType.push_back(std::move(astOrType))
		   :  memberType.push_back(type);
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
parseStructMemberList(std::vector<Token> &ident,
		      AstStructDecl::AstOrType &astOrType)
{
    if (token.kind != TokenKind::IDENTIFIER) {
	return false;
    }
    while (true) {
	if (token.kind != TokenKind::IDENTIFIER) {
	    error::out() << token.loc
		<< ": identifier for struct member expected" << std::endl;
	    error::fatal();
	    return false;
	}
	ident.push_back(token);
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
	astOrType = type;
	return true;
    } else if (auto ast = parseStructDeclaration()) {
	astOrType = std::move(ast);
	return true;
    } else {
	error::out() << token.loc << ": expected type or struct declaration"
	    << std::endl;
	error::fatal();
	return false;
    }
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
    auto type = Type::getSignedInteger(8 * sizeof(int));
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
static AstPtr parseDoWhileStatement();
static AstPtr parseForStatement();
static AstPtr parseReturnStatement();
static AstPtr parseBreakStatement();
static AstPtr parseContinueStatement();
static AstPtr parseExpressionStatement();
static AstPtr parseGotoStatement();
static AstPtr parseLabelDefinition();

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
    || (ast = parseSwitchStatement())
    || (ast = parseWhileStatement())
    || (ast = parseDoWhileStatement())
    || (ast = parseForStatement())
    || (ast = parseReturnStatement())
    || (ast = parseBreakStatement())
    || (ast = parseContinueStatement())
    || (ast = parseExpressionStatement())
    || (ast = parseGotoStatement())
    || (ast = parseLabelDefinition());
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

    (ast = parseTypeDeclaration())
	|| (ast = parseEnumDeclaration())
	|| (ast = parseStructDeclaration())
	|| (ast = parseGlobalVariableDefinition())
	|| (ast = parseLocalVariableDefinition())
	;
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
 * do-while-statement = "do" compound-statement "while" "(" expression ")"
 */
static AstPtr
parseDoWhileStatement()
{
    if (token.kind != TokenKind::DO) {
	return nullptr;
    }
    getToken();
    auto doWhileBody = parseCompoundStatement();
    if (!doWhileBody) {
	error::out() << token.loc << ": compound statement expected"
	    << std::endl;
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
    auto doWhileCond = parseExpression();
    if (!doWhileCond) {
	error::out() << token.loc << ": expression expected" << std::endl;
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
    AstPtr forInitDecl;
    ExprPtr forInitExpr;
    Symtab::openScope();
    if (!(forInitDecl = parseLocalVariableDefinition())) {
	if (!(forInitDecl = parseGlobalVariableDefinition())) {
	    forInitExpr = parseExpression();
	    if (!error::expected(TokenKind::SEMICOLON)) {
		Symtab::closeScope();
		return nullptr;
	    }
	    getToken();
	}
    }
    auto forCond = parseExpression();
    if (!error::expected(TokenKind::SEMICOLON)) {
	Symtab::closeScope();
	return nullptr;
    }
    getToken();
    auto forUpdate = parseExpression();
    if (!error::expected(TokenKind::RPAREN)) {
	Symtab::closeScope();
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
    auto forBody = parseCompoundStatement(false);
    if (!forBody) {
	error::out() << token.loc << ": compound statement expected"
	    << std::endl;
	error::fatal();
	Symtab::closeScope();
	return nullptr;
    }
    forLoop->appendBody(std::move(forBody));
    Symtab::closeScope();
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
    auto loc = token.loc;
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstBreak>(loc);
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
    auto loc = token.loc;
    getToken();
    if (!error::expected(TokenKind::SEMICOLON)) {
	return nullptr;
    }
    getToken();
    return std::make_unique<AstContinue>(loc);
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
	if (auto type = Symtab::getNamedType(token.val, Symtab::AnyScope)) {
	    getToken();
	    return type;
	}
	return nullptr;
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
static const Type * parseArrayDimAndType();

/*
 * array-type = "array" array-dim-and-type
 */
static const Type *
parseArrayType()
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    auto type = parseArrayDimAndType();
    if (!type) {
	error::out() << token.loc << ": array dimension expected" << std::endl;
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
parseArrayDimAndType()
{
    if (token.kind != TokenKind::LBRACKET) {
	return nullptr;
    }
    getToken();
    auto dimExpr = parseExpression();
    if (!dimExpr || !dimExpr->isConst() || !dimExpr->type->isInteger()) {
	error::out() << token.loc
	    << ": constant integer expression expected for array dimension"
	    << std::endl;
	error::fatal();
	return nullptr;
    }
    auto dim = dimExpr->getSignedIntValue();
    if (dim < 0) {
	error::out() << token.loc << ": dimension can not be negative"
	    << std::endl;
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
	    error::out() << token.loc << ": type expected" << std::endl;
	    error::fatal();
	}
	return Type::getArray(type, dim);
    } else {
	auto type = parseArrayDimAndType();
	if (!type) {
	    error::out() << token.loc
		<< ": expected 'of' or array dimension" << std::endl;
	    error::fatal();
	    return nullptr;
	}
	return Type::getArray(type, dim);
    }
}
