#include <sstream>
#include <iostream>
#include <unordered_map>

#include "lexer.hpp"
#include "parser.hpp"
#include "parseexpr.hpp"
#include "symtab.hpp"


static Binary::Kind
getBinaryExprKind(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK:
	case TokenKind::ASTERISK_EQUAL:
	    return Binary::Kind::MUL;
	case TokenKind::SLASH:
	case TokenKind::SLASH_EQUAL:
	    return Binary::Kind::DIV;
	case TokenKind::PERCENT:
	case TokenKind::PERCENT_EQUAL:
	    return Binary::Kind::MOD;
	case TokenKind::PLUS:
	case TokenKind::PLUS2:
	case TokenKind::PLUS_EQUAL:
	    return Binary::Kind::ADD;
	case TokenKind::MINUS:
	case TokenKind::MINUS2:
	case TokenKind::MINUS_EQUAL:
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

//------------------------------------------------------------------------------

static ExprPtr parseAssignment(void);
static ExprPtr parseConditional(void);
static ExprPtr parseBinary(int prec);
static ExprPtr parsePrefix(void);
static ExprPtr parsePostfix(ExprPtr &&expr);
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
	semanticError(loc, "constant expression required");
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
    while (token.kind == TokenKind::EQUAL
        || token.kind == TokenKind::PLUS_EQUAL
        || token.kind == TokenKind::MINUS_EQUAL
        || token.kind == TokenKind::ASTERISK_EQUAL
        || token.kind == TokenKind::SLASH_EQUAL
        || token.kind == TokenKind::PERCENT_EQUAL)
    {
	auto opTok = token;
        getToken();
	
	auto right = parseAssignment();
	if (!right) {
	    expectedError("assignment expression");
	}
	if (!expr->isLValue()) {
	    semanticError(opTok.loc,
			  "lvalue required as left operand of assignment");
	}
	switch (opTok.kind) {
	    case TokenKind::EQUAL:
		expr = Expr::createBinary(Binary::Kind::ASSIGN, std::move(expr),
					  std::move(right));
		break;
	    case TokenKind::PLUS_EQUAL:
	    case TokenKind::MINUS_EQUAL:
	    case TokenKind::ASTERISK_EQUAL:
	    case TokenKind::SLASH_EQUAL:
	    case TokenKind::PERCENT_EQUAL:
		// <expr> += <right> becomes <expr> = <expr> + <right>,
		// <expr> -= <right> becomes <expr> = <expr> - <right>, etc.
		expr = Expr::createBinary(
			    Binary::ASSIGN,
			    std::move(expr),
			    Expr::createBinary(
				getBinaryExprKind(opTok.kind),
				Expr::createProxy(expr.get()),
				std::move(right)));
		break;
	    default:
		assert(0);
	}
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

static ExprPtr
parseBinary(int prec)
{
    auto expr = parsePrefix();
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
parsePrefix(void)
{
    if (token.kind != TokenKind::PLUS && token.kind != TokenKind::MINUS
     && token.kind != TokenKind::NOT && token.kind != TokenKind::AND
     && token.kind != TokenKind::ASTERISK && token.kind != TokenKind::PLUS2
     && token.kind != TokenKind::MINUS2)
    {
	return parsePostfix(parsePrimary());
    }

    auto opTok = token;
    getToken();
    auto expr = parsePrefix();
    if (!expr) {
	semanticError(opTok.loc, "non-empty expression expected");
    }
    switch (opTok.kind) {
	case TokenKind::MINUS:
	    expr = Expr::createUnaryMinus(std::move(expr));
	    break;
	case TokenKind::NOT:
	    expr = Expr::createLogicalNot(std::move(expr));
	    break;
	case TokenKind::ASTERISK:
	    if (!expr->getType()->isPointer() && !expr->getType()->isArray()) {
		semanticError(opTok.loc,
			      "'*' can only be applied to a pointer or array");
	    }
	    expr = Expr::createDeref(std::move(expr));
	    break;
	case TokenKind::AND:
	    if (!expr->isLValue()) {
		semanticError(opTok.loc,
			      "'&' can only be applied to an l-value");
	    }
	    expr = Expr::createAddr(std::move(expr));
	    break;
	case TokenKind::PLUS2:
	case TokenKind::MINUS2:
	    if (!expr->isLValue()) {
		semanticError(opTok.loc,
			      "'++' can only be applied to an l-value");
	    }
	    // ++<expr> becomes <expr> = <expr> + 1
	    // --<expr> becomes <expr> = <expr> - 1
	    expr = Expr::createBinary(
			Binary::ASSIGN,
			std::move(expr),
			Expr::createBinary(
			    getBinaryExprKind(opTok.kind),
			    Expr::createProxy(expr.get()),
			    Expr::createLiteral("1", 10)));
	    break;
	default:
	    assert(0);
    }
    return expr;
} 

static ExprPtr
parsePostfix(ExprPtr &&expr)
{
    if (token.kind != TokenKind::LPAREN && token.kind != TokenKind::LBRACKET
     && token.kind != TokenKind::ARROW && token.kind != TokenKind::PLUS2
     && token.kind != TokenKind::MINUS2)
    {
	return expr;
    }

    auto opTok = token;
    getToken();

    switch (opTok.kind) {
	case TokenKind::LPAREN:
	    // function call
	    if (!expr->getType()->isFunction()) {
		// TODO: Store loc in expr
		semanticError(opTok.loc, "not a function");
	    } else {
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
		auto numArgs = expr->getType()->getArgType().size();
		if (numArgs != param.size()) {
		    std::stringstream msg;
		    msg << "function expects " << numArgs << " paramaters, not "
			<< param.size();
		    semanticError(msg.str().c_str());
		}

		expr = Expr::createCall(std::move(expr), std::move(param));
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::LBRACKET:
	    if (auto index = parseExpr()) {
		expected(TokenKind::RBRACKET);
		getToken();
		expr = Expr::createBinary(Binary::ADD, std::move(expr),
					  std::move(index));
		if (!expr->getType()->isPointer()){
		    // TODO: Store loc in expr
		    semanticError(opTok.loc,
			      "subscripted value is neither array nor pointer");
		}
		expr = Expr::createDeref(std::move(expr));
		return parsePostfix(std::move(expr));
	    } else {
		semanticError(opTok.loc, "non-empty expression expected");
	    }
	case TokenKind::ARROW:
	    if (!expr->getType()->isPointer()) {
		semanticError(opTok.loc,
			      "'->' can only be applied to a pointer or array");
	    }
	    return parsePostfix(Expr::createDeref(std::move(expr)));
	case TokenKind::PLUS2:
	case TokenKind::MINUS2:
	    if (!expr->isLValue()) {
		semanticError(opTok.loc,
			      "'&' can only be applied to an l-value");
	    } else {
		auto binaryOp = opTok.kind == TokenKind::PLUS2
		    ? Binary::POSTFIX_INC
		    : Binary::POSTFIX_DEC;
		auto one = Expr::createLiteral("1", 10);
		return Expr::createBinary(binaryOp, std::move(expr),
					  std::move(one));
	    }
	default:
	    ;
    }
    assert(0);
    return nullptr;
}

static ExprPtr
parsePrimary(void)
{
    if (token.kind == TokenKind::IDENTIFIER) {
	auto identTok = token;
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
    } else if (token.kind == TokenKind::STRING_LITERAL) {
	auto val = token.valProcessed.c_str();
        getToken();
	static std::size_t id;
	std::stringstream ss;
	ss << ".str" << id;
	UStr ident(ss.str());

	gen::defStringLiteral(ident.c_str(), val, true);
	auto ty = Type::getUnsignedInteger(8);
	auto expr = Expr::createAddr(Expr::createIdentifier(ident.c_str(), ty));
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
