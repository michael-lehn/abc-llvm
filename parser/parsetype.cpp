#include <vector>

#include "lexer/error.hpp"
#include "lexer/lexer.hpp"

#include "parseexpr.hpp"
#include "parsetype.hpp"

namespace abc {

using namespace lexer;

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
	auto typeNode = parseType_(false);
	if (!typeNode) {
	    error::fatal(token.loc, "type for parameter expected");
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
	fnRetType = parseType_(false);
	if (!fnRetType) {
	    error::fatal(token.loc, "return type for function expected");
	    return nullptr;
	}
    }
    return std::make_unique<FunctionTypeNode>(std::move(fnName),
					      std::move(fnParamName),
					      std::move(fnParamType),
					      fnHasVargs,
					      std::move(fnRetType));
}

static TypeNodePtr
parseArrayType(bool allowZeroDim)
{
    if (token.kind != TokenKind::ARRAY) {
	return nullptr;
    }
    getToken();
    std::vector<ExprPtr> dimExpr;
    for (bool first = true; token.kind == TokenKind::LBRACKET; first = false) {
	getToken();
	dimExpr.push_back(parseAssignmentExpression());
	if (!dimExpr.back() && (!first || !allowZeroDim)) {
	    error::fatal(token.loc, "dimension expected");
	}
	error::expected(TokenKind::RBRACKET);
	getToken();
    }
    error::expected(TokenKind::OF);
    getToken();

    auto refTypeNode = parseType_(false);
    if (!refTypeNode) {
	error::fatal(token.loc, "element type for array expected");
    }
    return std::make_unique<ArrayTypeNode>(std::move(dimExpr),
					   std::move(refTypeNode));
}

static TypeNodePtr
parsePointerType()
{
    if (token.kind != TokenKind::ARROW) {
	return nullptr;
    }
    getToken();
    auto refTypeNode = parseType_(false);
    if (!refTypeNode) {
	error::fatal(token.loc, "target type for pointer expected");
	return nullptr;
    }
    return std::make_unique<PointerTypeNode>(std::move(refTypeNode));
}

TypeNodePtr
parseUnqualifiedType(bool allowZeroDim)
{
    if (token.kind == TokenKind::IDENTIFIER) {
	auto tok = token;
	getToken();
	return std::make_unique<IdentifierTypeNode>(tok);
    } else if (auto typeNode = parsePointerType()) {
	return typeNode;
    } else if (auto typeNode = parseArrayType(allowZeroDim)) {
	return typeNode;
    } else if (auto typeNode = parseFunctionType()) {
	return typeNode;
    } else {
	return nullptr;
    }
}

TypeNodePtr
parseType_(bool allowZeroDim)
{
    bool isReadonlyType = false;
    auto tok = token;
    if (token.kind == TokenKind::CONST) {
	getToken();
	isReadonlyType = true;
    }
    auto typeNode = parseUnqualifiedType(allowZeroDim);
    if (!typeNode && isReadonlyType) {
	error::fatal(tok.loc, "unqualified type expected after 'readonly'");
    }
    return isReadonlyType
	? std::make_unique<ReadonlyTypeNode>(std::move(typeNode))
	: std::move(typeNode);
}

} // namespace abc
