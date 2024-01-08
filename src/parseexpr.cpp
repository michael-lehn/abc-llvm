#include <sstream>
#include <iostream>
#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "parseexpr.hpp"
#include "symtab.hpp"

static ExprPtr parseAssignment(void);
static ExprPtr parseBinary(int prec);
static ExprPtr parseUnary(void);
static ExprPtr parsePrimary(void);

ExprPtr
parseExpr(void)
{
    return parseAssignment();
}

ExprPtr
parseConstExpr(void)
{
    auto loc = token.loc;
    auto expr = parseExpr();
    if (!isConst(expr.get())) {
	semanticError(loc, "constant expression");
	return nullptr;
    }
    return expr;
}

static ExprPtr
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
	expr = getBinaryExpr(BinaryExprKind::ASSIGN, std::move(expr),
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

static ExprPtr
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
	    expr = getBinaryExpr(op, std::move(expr), std::move(right));
        }
    }
    return expr;
}

static ExprPtr
parseUnary(void)
{
    if (token.kind == TokenKind::PLUS || token.kind == TokenKind::MINUS) {
        getToken();
	auto expr = parseUnary();
        if (!expr) {
            expectedError("non-empty expression");
        }
	if (token.kind == TokenKind::MINUS) {
	    expr = getUnaryMinusExpr(std::move(expr));
	}
    }
    return parsePrimary();
} 

static ExprPtr
parsePrimary(void)
{
    if (token.kind == TokenKind::IDENTIFIER) {
	auto symEntry = symtab::get(token.val.c_str());
	if (!symEntry) {
	    std::string msg = "undeclared identifier '";
	    msg += token.val.c_str();
	    msg += "'";
	    semanticError(msg.c_str());
	}
        getToken();
	auto expr = getIdentifierExpr(symEntry->internalIdent.c_str());
	if (token.kind == TokenKind::LPAREN) {
	    // function call
	    if (!symEntry->type->isFunction()) {
		std::string msg = "'";
		msg += token.val.c_str();
		msg += "' is not a function";
		semanticError(msg.c_str());
	    }
	    getToken();

	    // parse parameter list
	    ExprVector param;
	    while (auto p = parseExpr()) {
		param.push_back(std::move(p));
		if (token.kind != TokenKind::COMMA) {
		    break;
		}
		getToken();
	    }
	    expected(TokenKind::RPAREN);
	    getToken();

	    // check parameters
	    auto numArgs = symEntry->type->getArgType().size();
	    if (numArgs != param.size()) {
		std::stringstream msg;
		msg << "function '" << symEntry->ident.c_str() << "' expects "
		    << numArgs << " paramaters, not " << param.size();
		semanticError(msg.str().c_str());
	    }

	    expr = getCallExpr(std::move(expr), std::move(param));
	}
        return expr;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
	auto expr = getLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	// TODO: hex!
	auto expr = getLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	// TODO: oct!
	auto expr = getLiteralExpr(token.val.c_str());
        getToken();
        return expr;
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
	auto expr = parseExpr();
        if (!expr) {
            expectedError("expression");
	}
        expected(TokenKind::RPAREN);
        getToken();
        return expr;
    }
    return nullptr;
} 
