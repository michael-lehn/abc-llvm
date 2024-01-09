#include <sstream>
#include <iostream>
#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "parseexpr.hpp"
#include "symtab.hpp"

static ExprPtr parseAssignment(void);
static ExprPtr parseConditional(void);
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
    if (!expr->isConst()) {
	semanticError(loc, "constant expression");
	return nullptr;
    }
    return expr;
}

static ExprPtr
parseAssignment(void)
{
    auto expr = parseConditional();
    if (!expr) {
	return nullptr;
    }
    while (token.kind == TokenKind::EQUAL) {
	getToken();
	auto right = parseAssignment();
	if (!right) {
	    expectedError("assignment expression");
	}
	expr = Expr::createBinary(Binary::Kind::ASSIGN, std::move(expr),
				  std::move(right));
    }
    return expr;
}

static ExprPtr
parseConditional(void)
{
    auto expr = parseBinary(1);
    if (!expr) {
	return nullptr;
    }

    if (token.kind == TokenKind::QUERY) {
	getToken();
	
	auto left = parseAssignment();
	if (!left) {
	    expectedError("non-empty expression");
	}
	expected(TokenKind::COLON);
	getToken();
	auto right = parseConditional();
	if (!right) {
	    expectedError("non-empty expression");
	}
	expr = Expr::createConditional(std::move(expr), std::move(left),
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

    { TokenKind::AND2, 5},

    { TokenKind::OR2, 4},

    { TokenKind::NOT_EQUAL, 9},

};

static int
tokenKindPrec(TokenKind kind)
{
    return binaryOpPrec.contains(kind) ? binaryOpPrec[kind] : 0;
}

static Binary::Kind
getBinaryExprKind(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK:
	    return Binary::Kind::MUL;
	case TokenKind::SLASH:
	    return Binary::Kind::DIV;
	case TokenKind::PERCENT:
	    return Binary::Kind::MOD;
	case TokenKind::PLUS:
	    return Binary::Kind::ADD;
	case TokenKind::MINUS:
	    return Binary::Kind::SUB;
	case TokenKind::EQUAL2:
	    return Binary::Kind::EQUAL;
	case TokenKind::NOT_EQUAL:
	    return Binary::Kind::NOT_EQUAL;
	case TokenKind::LESS:
	    return Binary::Kind::LESS;
	case TokenKind::LESS_EQUAL:
	    return Binary::Kind::LESS_EQUAL;
	case TokenKind::GREATER:
	    return Binary::Kind::GREATER;
	case TokenKind::GREATER_EQUAL:
	    return Binary::Kind::GREATER_EQUAL;
	case TokenKind::AND2:
	    return Binary::Kind::LOGICAL_AND;
	case TokenKind::OR2:
	    return Binary::Kind::LOGICAL_OR;
	default:
	    assert(0);
	    return Binary::Kind::ADD; // never reached
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
	    expr = Expr::createBinary(op, std::move(expr), std::move(right));
        }
    }
    return expr;
}

static ExprPtr
parseUnary(void)
{
    if (token.kind == TokenKind::PLUS || token.kind == TokenKind::MINUS
     || token.kind == TokenKind::NOT)
    {
	auto op = token.kind;
        getToken();
	auto expr = parseUnary();
        if (!expr) {
            expectedError("non-empty expression");
        }
	if (op == TokenKind::MINUS) {
	    expr = Expr::createUnaryMinus(std::move(expr));
	} else if (op == TokenKind::NOT) {
	    expr = Expr::createLogicalNot(std::move(expr));
	}
	return expr;
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
	auto expr = Expr::createIdentifier(symEntry->internalIdent.c_str(),
					   symEntry->type);
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

	    expr = Expr::createCall(std::move(expr), std::move(param));
	}
        return expr;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType();
	auto expr = Expr::createLiteral(val, 10, ty);
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType();
	auto expr = Expr::createLiteral(val, 16, ty);
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType();
	auto expr = Expr::createLiteral(val, 8, ty);
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
