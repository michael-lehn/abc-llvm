#include <iostream>
#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "parseexpr.hpp"
#include "symtab.hpp"

static ExprUniquePtr parseAssignment(void);
static ExprUniquePtr parseBinary(int prec);
static ExprUniquePtr parseUnary(void);
static ExprUniquePtr parsePrimary(void);

ExprUniquePtr
parseExpr(void)
{
    return parseAssignment();
}

static ExprUniquePtr
parseAssignment(void)
{
    auto expr = parseBinary(1);
    if (!expr) {
	return nullptr;
    }
    while (token.kind == TokenKind::EQUAL) {
	getToken();
	auto right = parseAssignment();
	if (!right) {
	    expectedError("assignment expression");
	}
	expr = makeBinaryExpr(BinaryExprKind::ASSIGN, std::move(expr),
			      std::move(right));
    }
    return expr;
}

static std::unordered_map<TokenKind, int> binaryOpPrec = {
    { TokenKind::ASTERISK, 13},
    { TokenKind::SLASH, 13},
    { TokenKind::PERCENT, 13},

    { TokenKind::PLUS, 11},
    { TokenKind::MINUS, 11},

    { TokenKind::GREATER, 10},
    { TokenKind::GREATER_EQUAL, 10},
    { TokenKind::LESS, 10},
    { TokenKind::LESS_EQUAL, 10},

    { TokenKind::EQUAL2, 9},
    { TokenKind::NOT_EQUAL, 9},

};

static int
tokenKindPrec(TokenKind kind)
{
    return binaryOpPrec.contains(kind) ? binaryOpPrec[kind] : 0;
}

static BinaryExprKind
getBinaryExprKind(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK:
	    return BinaryExprKind::MUL;
	case TokenKind::SLASH:
	    return BinaryExprKind::DIV;
	case TokenKind::PERCENT:
	    return BinaryExprKind::MOD;
	case TokenKind::PLUS:
	    return BinaryExprKind::ADD;
	case TokenKind::MINUS:
	    return BinaryExprKind::SUB;
	case TokenKind::EQUAL2:
	    return BinaryExprKind::EQUAL;
	case TokenKind::NOT_EQUAL:
	    return BinaryExprKind::NOT_EQUAL;
	case TokenKind::LESS:
	    return BinaryExprKind::LESS;
	case TokenKind::LESS_EQUAL:
	    return BinaryExprKind::LESS_EQUAL;
	case TokenKind::GREATER:
	    return BinaryExprKind::GREATER;
	case TokenKind::GREATER_EQUAL:
	    return BinaryExprKind::GREATER_EQUAL;
	default:
	    assert(0);
	    return BinaryExprKind::ADD; // never reached
    }
}

static ExprUniquePtr
parseBinary(int prec)
{
    auto expr = parseUnary();
    if (!expr) {
	return nullptr;
    }
    for (int p = tokenKindPrec(token.kind); p >= prec; --p) {
        while (tokenKindPrec(token.kind) == p) {
	    auto op = getBinaryExprKind(token.kind);
            getToken();
	    auto right = parseBinary(p + 1);
            if (!expr) {
                expectedError("non-empty expression");
            }
	    expr = makeBinaryExpr(op, std::move(expr), std::move(right));
        }
    }
    return expr;
}

static ExprUniquePtr
parseUnary(void)
{
    if (token.kind == TokenKind::PLUS || token.kind == TokenKind::MINUS) {
        getToken();
	auto expr = parseUnary();
        if (!expr) {
            expectedError("non-empty expression");
        }
	if (token.kind == TokenKind::MINUS) {
	    expr = makeUnaryMinusExpr(std::move(expr));
	}
    }
    return parsePrimary();
} 

static ExprUniquePtr
parsePrimary(void)
{
    if (token.kind == TokenKind::IDENTIFIER) {
	if (!symtab::get(token.val.c_str())) {
	    std::string msg = "undeclared identifier '";
	    msg += token.val.c_str();
	    msg += "'";
	    semanticError(msg.c_str());
	}
	auto expr = makeIdentifierExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
	auto expr = makeLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	// TODO: hex!
	auto expr = makeLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	// TODO: oct!
	auto expr = makeLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
	auto expr = parseAssignment();
        if (!expr) {
            expectedError("expression");
	}
        expected(TokenKind::RPAREN);
        getToken();
        return expr;
    }
    return nullptr;
} 
