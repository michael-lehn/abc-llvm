#include <sstream>
#include <iostream>
#include <unordered_map>

#include "asserthack.hpp"
#include "binaryexpr.hpp"
#include "callexpr.hpp"
#include "castexpr.hpp"
#include "conditionalexpr.hpp"
#include "compoundliteral.hpp"
#include "error.hpp"
#include "exprvector.hpp"
#include "identifier.hpp"
#include "initializerlist.hpp"
#include "integerliteral.hpp"
#include "lexer.hpp"
#include "parseexpr.hpp"
#include "parser.hpp"
#include "stringliteral.hpp"
#include "symtab.hpp"
#include "unaryexpr.hpp"


static BinaryExpr::Kind
getBinaryExprKind(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK:
	case TokenKind::ASTERISK_EQUAL:
	    return BinaryExpr::Kind::MUL;
	case TokenKind::SLASH:
	case TokenKind::SLASH_EQUAL:
	    return BinaryExpr::Kind::DIV;
	case TokenKind::PERCENT:
	case TokenKind::PERCENT_EQUAL:
	    return BinaryExpr::Kind::MOD;
	case TokenKind::PLUS:
	case TokenKind::PLUS2:
	case TokenKind::PLUS_EQUAL:
	    return BinaryExpr::Kind::ADD;
	case TokenKind::MINUS:
	case TokenKind::MINUS2:
	case TokenKind::MINUS_EQUAL:
	    return BinaryExpr::Kind::SUB;
	case TokenKind::EQUAL2:
	    return BinaryExpr::Kind::EQUAL;
	case TokenKind::NOT_EQUAL:
	    return BinaryExpr::Kind::NOT_EQUAL;
	case TokenKind::LESS:
	    return BinaryExpr::Kind::LESS;
	case TokenKind::LESS_EQUAL:
	    return BinaryExpr::Kind::LESS_EQUAL;
	case TokenKind::GREATER:
	    return BinaryExpr::Kind::GREATER;
	case TokenKind::GREATER_EQUAL:
	    return BinaryExpr::Kind::GREATER_EQUAL;
	case TokenKind::AND2:
	    return BinaryExpr::Kind::LOGICAL_AND;
	case TokenKind::OR2:
	    return BinaryExpr::Kind::LOGICAL_OR;
	default:
	    assert(0);
	    return BinaryExpr::Kind::ADD; // never reached
    }
}

//------------------------------------------------------------------------------

static const Type *
parseIntType()
{
    auto type = parseType();
    if (type && !type->isInteger()) {
	return nullptr;
    }
    return type;
}

//------------------------------------------------------------------------------

static ExprPtr parseAssignment();
static ExprPtr parseConditional();
static ExprPtr parseBinary(int prec);
static ExprPtr parsePrefix();
static ExprPtr parsePostfix(ExprPtr &&expr);
static ExprPtr parsePrimary();


ExprPtr
parseExpression()
{
    return parseAssignment();
}

ExprPtr
parseConstExpr()
{
    auto loc = token.loc;
    auto expr = parseExpression();
    if (!expr || !expr->isConst()) {
	error::out() << loc
	    << " constant expression required" << std::endl;
	error::fatal();
	return nullptr;
    }
    return expr;
}

static ExprPtr
parseAssignment()
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
		expr = BinaryExpr::create(BinaryExpr::Kind::ASSIGN,
					  std::move(expr),
					  std::move(right), opTok.loc);
		break;
	    case TokenKind::PLUS_EQUAL:
	    case TokenKind::MINUS_EQUAL:
	    case TokenKind::ASTERISK_EQUAL:
	    case TokenKind::SLASH_EQUAL:
	    case TokenKind::PERCENT_EQUAL:
		// <expr> += <right> becomes <expr> = <expr> + <right>,
		// <expr> -= <right> becomes <expr> = <expr> - <right>, etc.
		expr = BinaryExpr::createOpAssign(
			    getBinaryExprKind(opTok.kind),
			    std::move(expr),
			    std::move(right),
			    opTok.loc);
		break;
	    default:
		assert(0);
	}
    }
    return expr;
}

static ExprPtr
parseConditional()
{
    auto expr = parseBinary(1);
    if (!expr) {
	return nullptr;
    }

    if (token.kind == TokenKind::QUERY || token.kind == TokenKind::THEN) {
	auto opLoc = token.loc;
	getToken();
	
	auto left = parseAssignment();
	if (!left) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	if (token.kind != TokenKind::COLON && token.kind != TokenKind::ELSE) {
	    error::out() << token.loc << " expected ':' or 'else'"
		<< std::endl;
	    error::fatal();
	}
	getToken();
	auto right = parseConditional();
	if (!right) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	expr = ConditionalExpr::create(std::move(expr), std::move(left),
				       std::move(right), opLoc);
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
            if (!right) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
            }
	    expr = BinaryExpr::create(op, std::move(expr), std::move(right),
				      opLoc);
        }
    }
    return expr;
}

static ExprPtr
parsePrefix()
{
    auto opLoc = token.loc;
    switch (token.kind) {
	case TokenKind::LPAREN:
	    getToken();
	    if (auto type = parseType()) {
		// it's a C cast
		if (!error::expected(TokenKind::RPAREN)) {
		    return nullptr;
		}
		getToken();
		if (auto ast = parseInitializerList(type)) {
		    return CompoundLiteral::create(std::move(ast), opLoc);
		}
		return CastExpr::create(parsePrefix(), type, opLoc, true);
	    } else {
		// ups, it was the '(' of a primary expression
		auto expr = parseExpression();
		if (!error::expected(TokenKind::RPAREN)) {
		    return nullptr;
		}
		getToken();
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::MINUS:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::MINUS,
				     parsePrefix(),
				     opLoc);
	case TokenKind::NOT:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::LOGICAL_NOT,
				     parsePrefix(),
				     opLoc);
	case TokenKind::ASTERISK:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::DEREF,
				     parsePrefix(),
				     opLoc);
	case TokenKind::AND:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::ADDRESS,
				     parsePrefix(),
				     opLoc);
	case TokenKind::PLUS2:
	    getToken();
	    return BinaryExpr::createOpAssign(BinaryExpr::ADD,
					      parsePrefix(),
					      IntegerLiteral::create(1),
					      opLoc);
	case TokenKind::MINUS2:
	    getToken();
	    return BinaryExpr::createOpAssign(BinaryExpr::SUB,
					      parsePrefix(),
					      IntegerLiteral::create(1),
					      opLoc);
	default:
	    return parsePostfix(parsePrimary());
    }
} 

static ExprPtr
parsePostfix(ExprPtr &&expr)
{
    if (token.kind != TokenKind::DOT && token.kind != TokenKind::LPAREN
     && token.kind != TokenKind::LBRACKET
     && token.kind != TokenKind::ARROW && token.kind != TokenKind::PLUS2
     && token.kind != TokenKind::MINUS2)
    {
	return expr;
    }

    auto opTok = token;
    getToken();

    if (!expr) {
	error::out() << opTok.loc
	    << ": postfix operator '" << tokenCStr(opTok.kind) << "' requires"
	    << " non-empty expression" << std::endl;
	error::fatal();

    }

    switch (opTok.kind) {
	case TokenKind::DOT:
	    // member access
	    {
		error::expected(TokenKind::IDENTIFIER);
		expr = BinaryExpr::createMember(std::move(expr), token.val,
						token.loc);
		getToken();
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::ARROW:
	    if (!expr->type->isPointer()) {
		error::out() << token.loc
		    << "'->' can only be applied to a pointer or array"
		    << std::endl;
		error::fatal();
	    }
	    expr = UnaryExpr::create(UnaryExpr::DEREF, std::move(expr),
				     opTok.loc);
	    expr = parsePostfix(std::move(expr));
	    if (token.kind == TokenKind::IDENTIFIER) {
		// member access
		expr = BinaryExpr::createMember(std::move(expr), token.val,
						token.loc);
		getToken();
		expr = parsePostfix(std::move(expr));
	    }
	    return expr;
	case TokenKind::LPAREN:
	    // function call
	    {
		// parse parameter list
		auto loc = token.loc;
		std::vector<ExprPtr> param;
		while (auto p = parseExpression()) {
		    param.push_back(std::move(p));
		    if (token.kind != TokenKind::COMMA) {
			break;
		    }
		    getToken();
		}
		error::expected(TokenKind::RPAREN);
		getToken();

		loc = combineLoc(expr->loc, token.loc);
		expr = CallExpr::create(std::move(expr), std::move(param), loc);
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::LBRACKET:
	    if (auto index = parseExpression()) {
		error::expected(TokenKind::RBRACKET);
		getToken();
		expr = BinaryExpr::create(BinaryExpr::ADD, std::move(expr),
					  std::move(index), opTok.loc);
		expr = UnaryExpr::create(UnaryExpr::DEREF, std::move(expr),
					 opTok.loc);
		return parsePostfix(std::move(expr));
	    } else {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
		return nullptr;
	    }
	case TokenKind::PLUS2:
	    if (!expr->isLValue()) {
		error::out() << token.loc
		    << "'++' can only be applied to an l-value" << std::endl;
		error::fatal();
	    }
	    return UnaryExpr::create(UnaryExpr::POSTFIX_INC, std::move(expr),
				     opTok.loc);
	case TokenKind::MINUS2:
	    if (!expr->isLValue()) {
		error::out() << token.loc
		    << "'--' can only be applied to an l-value" << std::endl;
		error::fatal();
	    }
	    return UnaryExpr::create(UnaryExpr::POSTFIX_DEC, std::move(expr),
				     opTok.loc);
	default:
	    ;
    }
    assert(0);
    return nullptr;
}

static ExprPtr
parsePrimary()
{
    auto opTok = token;
    if (token.kind == TokenKind::ASSERT) {
	// assert hack
	getToken();
	if (!error::expected(TokenKind::LPAREN)) {
	    return nullptr;
	}
	getToken();
	auto expr = parseExpression();
	if (!expr) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	if (!error::expected(TokenKind::RPAREN)) {
	    return nullptr;
	}
	getToken();
	return asserthack::createCall(std::move(expr), opTok.loc);
    } else if (token.kind == TokenKind::IDENTIFIER) {
	if (auto type = Symtab::getNamedType(token.val, Symtab::AnyScope)) {
	    getToken();
	    if (auto ast = parseInitializerList(type)) {
		return CompoundLiteral::create(std::move(ast), opTok.loc);
	    }
	    error::out() << token.loc
		<< ": error: initializer list expected" << std::endl;
	    error::fatal();
	    return nullptr;
	}
        getToken();
	auto expr = Identifier::create(opTok.val, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::SIZEOF) {
	getToken();
	if (!error::expected(TokenKind::LPAREN)) {
	    return nullptr;
	}
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
	    size = gen::getSizeOf(ty);
	} else {
	    auto expr = parseExpression();
	    if (!expr) {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
	    }
	    size = gen::getSizeOf(expr->type);
	}
	error::expected(TokenKind::RPAREN);
	getToken();

	std::stringstream ss;
	ss << size;
	auto ty = Type::getUnsignedInteger(64);
	auto val = UStr::create(ss.str());
	auto expr = IntegerLiteral::create(val, 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::NULLPTR) {
        getToken();
	auto ty = Type::getNullPointer();
	auto expr = IntegerLiteral::create(0, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::DECIMAL_LITERAL) {
	auto val = token.val;
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(val, 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	auto val = token.val;
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(val, 16, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	auto val = token.val;
        getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(val, 8, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::STRING_LITERAL) {
	std::string str{};
	std::string strRaw{};
	do {
	    str += token.val.c_str();
	    strRaw += token.valRaw.c_str();
	    getToken();
	} while (token.kind == TokenKind::STRING_LITERAL);
	auto expr = StringLiteral::create(UStr::create(str),
					  UStr::create(strRaw),
					  opTok.loc);	
	return expr;	
    } else if (token.kind == TokenKind::CHARACTER_LITERAL) {
	std::stringstream ss;
	ss << static_cast<int>(*token.val.c_str());
	auto val = UStr::create(ss.str());
        getToken();
	auto ty = Type::getSignedInteger(16);
	auto expr = IntegerLiteral::create(val, 10, ty, opTok.loc);
        return expr;
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
	auto expr = parseExpression();
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
