#include <vector>

#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"

#include "parseexpr.hpp"
#include "parsetype.hpp"

namespace abc {

using namespace lexer;

/*
 * struct-declaration = "struct" identifier (";" | { struct-member-list } )
 * struct-member-list
 *	= identifier { "," identifier } ":" ( type | struct-declaration ) ";"
 * 
 */
static TypeNodePtr
parseStructType()
{
    if (token.kind != TokenKind::STRUCT) {
	return nullptr;
    }
    getToken();
    error::expected(TokenKind::IDENTIFIER);
    auto structName = token;
    getToken();

    if (token.kind == TokenKind::SEMICOLON) {
	getToken();
	return std::make_unique<StructTypeNode>(structName);
    } else if (token.kind == TokenKind::LBRACE) {
	getToken();

	StructTypeNode::MemberDecl memberDecl;

	while (token.kind == TokenKind::IDENTIFIER) {
	    std::vector<lexer::Token> member;
	    do {
		error::expected(TokenKind::IDENTIFIER);
		member.push_back(token);
		getToken();
		if (token.kind == TokenKind::COLON) {
		    getToken();
		    break;
		}
		error::expected(TokenKind::COMMA);
		getToken();
	    } while (true);
	    auto typeNode = parseType_();
	    if (!typeNode) {
		error::fatal(token.loc, "type for member expected");
		return nullptr;
	    } else if (!typeNode->type()->hasSize()) {
		error::fatal(token.loc, "type has zero size");
		return nullptr;
	    }
	    error::expected(TokenKind::SEMICOLON);
	    getToken();
	    memberDecl.push_back({std::move(member), std::move(typeNode)});
	}
	error::expected(TokenKind::RBRACE);
	if (memberDecl.empty()) {
	    error::fatal(token.loc, "at least one membler required in struct");
	    return nullptr;
	}
	getToken();
	return std::make_unique<StructTypeNode>(structName,
					        std::move(memberDecl));
    } else {
	error::fatal(token.loc, "';' or '{' expected");
	return nullptr;
    }
}

/*
 * function-type
 *	= "fn" [identifier] "("
 *	   [ [identifier] ":" type { "," [identifier] ":" type} } ["," ...] ]
 *	   ")" [ ":" type ]
 */
static TypeNodePtr
parseFunctionType()
{
    if (token.kind != TokenKind::FN) {
	return nullptr;
    }
    getToken();

    std::optional<lexer::Token> fnName;
    std::vector<std::optional<lexer::Token>> fnParamName;
    std::vector<TypeNodePtr> fnParamType;
    bool fnHasVargs = false;
    TypeNodePtr fnRetType;

    if (token.kind == TokenKind::IDENTIFIER) {
	fnName = token;
	getToken();
    }
    error::expected(TokenKind::LPAREN);
    getToken();
    while (true) {
	if (token.kind == TokenKind::IDENTIFIER) {
	    fnParamName.push_back(token);
	    getToken();
	} else {
	    fnParamName.push_back(std::nullopt);
	}
	error::expected(TokenKind::COLON);
	getToken();
	auto typeNode = parseType_();
	if (!typeNode) {
	    error::fatal(token.loc, "type for parameter expected");
	    return nullptr;
	} else if (typeNode->type()->isUnboundArray()) {
	    error::fatal(token.loc, "use pointer instead of unbound array");
	    return nullptr;
	}
	fnParamType.push_back(std::move(typeNode));
	if (token.kind != TokenKind::COMMA) {
	    break;
	}
	getToken();
	if (token.kind == TokenKind::DOT3) {
	    getToken();
	    fnHasVargs = true;
	    break;
	}
    }
    error::expected(TokenKind::RPAREN);
    getToken();
    if (token.kind == TokenKind::COLON) {
	getToken();
	fnRetType = parseType_();
	if (!fnRetType) {
	    error::fatal(token.loc, "return type for function expected");
	    return nullptr;
	} else if (fnRetType->type()->isUnboundArray()) {
	    error::fatal(token.loc, "use pointer instead of unbound array");
	    return nullptr;
	}
    }
    return std::make_unique<FunctionTypeNode>(std::move(fnName),
					      std::move(fnParamName),
					      std::move(fnParamType),
					      fnHasVargs,
					      std::move(fnRetType));
}

/*
 * array-type = "array" "[" assignment-expression "]"
 *		{ "[" assignment-expression "]" }
 */
static TypeNodePtr
parseArrayType()
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    std::vector<ExprPtr> dimExpr;
    do {
	setTokenFix(TokenKind::LBRACKET);
	getToken();
	dimExpr.push_back(parseAssignmentExpression());
	setTokenFix(TokenKind::RBRACKET);
	getToken();
    } while (token.kind == TokenKind::LBRACKET);
    setTokenFix(TokenKind::OF);
    getToken();

    auto refTypeNode = parseType_();
    error::out() << "ok\n";
    return std::make_unique<ArrayTypeNode>(std::move(dimExpr),
					   std::move(refTypeNode));
}

/*
 * pointer-type = "->" type
 */
static TypeNodePtr
parsePointerType()
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto refTypeNode = parseType_();
    if (!refTypeNode) {
	error::fatal(token.loc, "target type for pointer expected");
	return nullptr;
    }
    return std::make_unique<PointerTypeNode>(std::move(refTypeNode));
}

/*
 * unqualified-type = identifier
 *		    | pointer-type
 *		    | array-type
 *		    | function-type
 *		    | struct-type
 */
TypeNodePtr
parseUnqualifiedType()
{
    if (token.kind == TokenKind::IDENTIFIER
	    && Symtab::type(token.val, Symtab::AnyScope))
    {
	auto tok = token;
	getToken();
	return std::make_unique<IdentifierTypeNode>(tok);
    } else if (auto typeNode = parsePointerType()) {
	return typeNode;
    } else if (auto typeNode = parseArrayType()) {
	return typeNode;
    } else if (auto typeNode = parseFunctionType()) {
	return typeNode;
    } else if (auto typeNode = parseStructType()) {
	return typeNode;
    } else {
	return nullptr;
    }
}

/*
 * type = [ "readonly" ] unqualified-type
 */
TypeNodePtr
parseType_()
{
    bool isReadonlyType = false;
    auto tok = token;
    if (token.kind == TokenKind::READONLY) {
	getToken();
	isReadonlyType = true;
    }
    auto typeNode = parseUnqualifiedType();
    if (!typeNode && isReadonlyType) {
	error::fatal(tok.loc, "unqualified type expected after 'readonly'");
    }
    return isReadonlyType
	? std::make_unique<ReadonlyTypeNode>(std::move(typeNode))
	: std::move(typeNode);
}

} // namespace abc
