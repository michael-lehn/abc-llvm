#include <sstream>
#include <iostream>
#include <unordered_map>

#include "error.hpp"
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
    if (!expr || !expr->isConst()) {
	error::out() << loc
	    << " constant expression required" << std::endl;
	error::fatal();
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
	    error::out() << token.loc
		<< " expected non-empty assignment expression" << std::endl;
	    error::fatal();
	}
	switch (opTok.kind) {
	    case TokenKind::EQUAL:
		expr = Expr::createBinary(Binary::Kind::ASSIGN, std::move(expr),
					  std::move(right), opTok.loc);
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
				std::move(right),
				opTok.loc),
			    opTok.loc);
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
	auto queryOpLoc = token.loc;
	getToken();
	
	auto left = parseAssignment();
	if (!left) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	error::expected(TokenKind::COLON);
	auto colonOpLoc = token.loc;
	getToken();
	auto right = parseConditional();
	if (!right) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	expr = Expr::createConditional(std::move(expr), std::move(left),
				       std::move(right), queryOpLoc,
				       colonOpLoc);
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
	    auto opLoc = token.loc;
	    auto op = getBinaryExprKind(token.kind);
            getToken();
	    auto right = parseBinary(p + 1);
            if (!expr) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
            }
	    expr = Expr::createBinary(op, std::move(expr), std::move(right),
				      opLoc);
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
	error::out() << token.loc << " expected non-empty expression"
	    << std::endl;
	error::fatal();
    }
    switch (opTok.kind) {
	case TokenKind::MINUS:
	    expr = Expr::createUnaryMinus(std::move(expr),opTok.loc);
	    break;
	case TokenKind::NOT:
	    expr = Expr::createLogicalNot(std::move(expr),opTok.loc);
	    break;
	case TokenKind::ASTERISK:
	    expr = Expr::createDeref(std::move(expr),opTok.loc);
	    break;
	case TokenKind::AND:
	    expr = Expr::createAddr(std::move(expr),opTok.loc);
	    break;
	case TokenKind::PLUS2:
	case TokenKind::MINUS2:
	    if (!expr->isLValue()) {
		error::out() << opTok.loc
		    << "'++' can only be applied to an l-value" << std::endl;
		error::fatal();
	    }
	    // ++<expr> becomes <expr> = <expr> + 1
	    // --<expr> becomes <expr> = <expr> - 1
	    expr = Expr::createBinary(
			Binary::ASSIGN,
			std::move(expr),
			Expr::createBinary(
			    getBinaryExprKind(opTok.kind),
			    Expr::createProxy(expr.get()),
			    Expr::createLiteral("1", 10),
			    opTok.loc),
			opTok.loc);
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
	    {
		// parse parameter list
		ExprVector param;
		while (auto p = parseExpr()) {
		    param.push_back(std::move(p));
		    if (token.kind != TokenKind::COMMA) {
			break;
		    }
		    getToken();
		}
		error::expected(TokenKind::RPAREN);
		getToken();

		expr = Expr::createCall(std::move(expr), std::move(param),
					opTok.loc);
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::LBRACKET:
	    if (auto index = parseExpr()) {
		error::expected(TokenKind::RBRACKET);
		getToken();
		expr = Expr::createBinary(Binary::ADD, std::move(expr),
					  std::move(index), opTok.loc);
		if (!expr->getType()->isPointer()){
		    error::out() << opTok.loc <<
			"subscripted value is neither array nor pointer"
			<< std::endl;
		    error::fatal();
		}
		expr = Expr::createDeref(std::move(expr));
		return parsePostfix(std::move(expr));
	    } else {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
	    }
	case TokenKind::ARROW:
	    if (!expr->getType()->isPointer()) {
		error::out() << token.loc
		    << "'->' can only be applied to a pointer or array"
		    << std::endl;
		error::fatal();
	    }
	    return parsePostfix(Expr::createDeref(std::move(expr), opTok.loc));
	case TokenKind::PLUS2:
	case TokenKind::MINUS2:
	    if (!expr->isLValue()) {
		error::out() << token.loc
		    << "'&' can only be applied to an l-value" << std::endl;
		error::fatal();
	    } else {
		auto binaryOp = opTok.kind == TokenKind::PLUS2
		    ? Binary::POSTFIX_INC
		    : Binary::POSTFIX_DEC;
		auto one = Expr::createLiteral("1", 10);
		return Expr::createBinary(binaryOp, std::move(expr),
					  std::move(one), opTok.loc);
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
    auto opTok = token;
    if (token.kind == TokenKind::IDENTIFIER) {
        getToken();
	auto expr = Expr::createIdentifier(opTok.val.c_str(), opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::SIZEOF) {
	getToken();
	error::expected(TokenKind::LPAREN);
	getToken();
	std::size_t size;
	if (token.kind == TokenKind::COLON) {
	    getToken();
	    auto loc = token.loc;
	    auto ty = parseType();
	    if (!ty) {
		error::out() << loc << " type expected" << std::endl;
		error::fatal();
	    }
	    size = Type::getSizeOf(ty);
	} else {
	    auto expr = parseExpr();
	    if (!expr) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
	    }
	    size = Type::getSizeOf(expr->getType());
	}
	error::expected(TokenKind::RPAREN);
	getToken();

	std::stringstream ss;
	ss << size;
	// TODO: Type 'ty' should be 'size_t'
	auto ty = Type::getUnsignedInteger(64);
	auto val = UStr{ss.str()}.c_str();
	auto expr = Expr::createLiteral(val, 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::NULLPTR) {
        getToken();
	auto ty = Type::getPointer(Type::getUnsignedInteger(8));
	auto expr = Expr::createLiteral("0", 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = Expr::createLiteral(val, 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = Expr::createLiteral(val, 16, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	auto val = token.val.c_str();
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = Expr::createLiteral(val, 8, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::STRING_LITERAL) {
	auto val = token.valProcessed.c_str();
        getToken();
	auto sym = symtab::addStringLiteral(val).c_str();
	auto expr = Expr::createIdentifier(sym, opTok.loc);	
	return expr;	
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
	auto expr = parseExpr();
        if (!expr) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	error::expected(TokenKind::RPAREN);
        getToken();
        return expr;
    }
    return nullptr;
} 
