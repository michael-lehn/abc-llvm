#include "expr/binaryexpr.hpp"
#include "expr/callexpr.hpp"
#include "expr/identifier.hpp"
#include "expr/integerliteral.hpp"
#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"
#include "type/type.hpp"
#include "type/integertype.hpp"

#include "parseexpr.hpp"
#include "parser.hpp"

namespace abc {

using namespace lexer;
    
static ExprPtr parseAssignment();
static ExprPtr parseBinary(int prec);
static ExprPtr parsePrefix();
static ExprPtr parsePostfix(ExprPtr &&expr);
static ExprPtr parsePrimary();

ExprPtr
parseExpression()
{
    return parseAssignment();
}

//------------------------------------------------------------------------------
static BinaryExpr::Kind
getBinaryExprKind(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK:
	    return BinaryExpr::Kind::MUL;
	case TokenKind::SLASH:
	    return BinaryExpr::Kind::DIV;
	case TokenKind::PERCENT:
	    return BinaryExpr::Kind::MOD;
	case TokenKind::PLUS:
	case TokenKind::PLUS2:
	    return BinaryExpr::Kind::ADD;
	case TokenKind::MINUS:
	case TokenKind::MINUS2:
	    return BinaryExpr::Kind::SUB;
	case TokenKind::EQUAL:
	    return BinaryExpr::Kind::ASSIGN;
	case TokenKind::PLUS_EQUAL:
	    return BinaryExpr::Kind::ADD_ASSIGN;
	case TokenKind::MINUS_EQUAL:
	    return BinaryExpr::Kind::SUB_ASSIGN;
	case TokenKind::ASTERISK_EQUAL:
	    return BinaryExpr::Kind::MUL_ASSIGN;
	case TokenKind::SLASH_EQUAL:
	    return BinaryExpr::Kind::DIV_ASSIGN;
	case TokenKind::PERCENT_EQUAL:
	    return BinaryExpr::Kind::MOD_ASSIGN;
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

static ExprPtr
parseAssignment()
{
    auto expr = parseBinary(1);
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
	auto tok = token;
        getToken();
	
	auto right = parseAssignment();
	if (!right) {
	    error::out() << token.loc
		<< " expected non-empty assignment expression" << std::endl;
	    error::fatal();
	}
	expr = BinaryExpr::create(getBinaryExprKind(tok.kind),
				  std::move(expr),
				  std::move(right), tok.loc);
    }
    return expr;
}

//------------------------------------------------------------------------------

static int
tokenKindPrec(TokenKind kind)
{
    switch (kind) {
	case TokenKind::ASTERISK: return 13;
	case TokenKind::SLASH: return 13;
	case TokenKind::PERCENT: return 13;
	case TokenKind::PLUS: return 11;
	case TokenKind::MINUS: return 11;
	case TokenKind::GREATER: return 10;
	case TokenKind::GREATER_EQUAL: return 10;
	case TokenKind::LESS: return 10;
	case TokenKind::LESS_EQUAL: return 10;
	case TokenKind::EQUAL2: return 9;
	case TokenKind::NOT_EQUAL: return 9;
	case TokenKind::AND2: return 5;
	case TokenKind::OR2: return 4;
	default: return 0;
    }
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
		error::out() << token.loc
		    << ": error: expected non-empty expression\n";
		error::fatal();
            }
	    expr = BinaryExpr::create(op, std::move(expr), std::move(right),
				      opLoc);
        }
    }
    return expr;
}

//------------------------------------------------------------------------------

static ExprPtr
parsePrefix()
{
    switch (token.kind) {
	case TokenKind::LPAREN:
	    getToken();
	    if (auto type = parseType()) {
		// it's a C cast
		assert(0 && "Not implemented");
		return nullptr;
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
	case TokenKind::NOT:
	case TokenKind::ASTERISK:
	case TokenKind::AND:
	case TokenKind::PLUS2:
	case TokenKind::MINUS2:
	    assert(0 && "Not implemented");
	    return nullptr;
	default:
	    return parsePostfix(parsePrimary());
    }
} 

//------------------------------------------------------------------------------

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

    auto tok = token;
    getToken();

    if (!expr) {
	error::out() << tok.loc
	    << ": postfix operator '" << tok.kind << "' requires"
	    << " non-empty expression" << std::endl;
	error::fatal();

    }

    switch (tok.kind) {
	case TokenKind::DOT:
	    // member access
	    assert(0 && "Not implemented");
	    return nullptr;
	case TokenKind::ARROW:
	    assert(0 && "Not implemented");
	    return nullptr;
	case TokenKind::LPAREN:
	    // function call
	    {
		// parse argument list
		std::vector<ExprPtr> arg;
		while (auto a = parseExpression()) {
		    arg.push_back(std::move(a));
		    if (token.kind != TokenKind::COMMA) {
			break;
		    }
		    getToken();
		}
		error::expected(TokenKind::RPAREN);
		getToken();
		expr = CallExpr::create(std::move(expr), std::move(arg),
				        tok.loc);
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::LBRACKET:
	    assert(0 && "Not implemented");
	    return nullptr;
	case TokenKind::PLUS2:
	    assert(0 && "Not implemented");
	    return nullptr;
	case TokenKind::MINUS2:
	    assert(0 && "Not implemented");
	    return nullptr;
	default:
	    assert(0);
	    return nullptr;
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

static ExprPtr
parsePrimary()
{
    auto tok = token;
    if (tok.kind == TokenKind::IDENTIFIER) {
	getToken();
	if (auto type = Symtab::type(tok.val, Symtab::AnyScope)) {
	    assert(0 && "Not implemented");
	    return nullptr;
	} else if (auto var = Symtab::variable(tok.val, Symtab::AnyScope)) {
	    auto ty = var->type;
	    auto expr = Identifier::create(tok.val, var->id, ty, tok.loc);
	    return expr;
	} else {
	    error::out() << tok.loc << ": error undefined identifier\n";
	    error::fatal();
	    return nullptr;
	}
    } else if (tok.kind == TokenKind::DECIMAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.val, 10, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.val, 16, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.val, 8, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::CHARACTER_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	if (!ty) {
	    ty = IntegerType::createChar();
	}
	auto val = *tok.processedVal.c_str();
	auto expr = IntegerLiteral::create(val, ty, tok.loc);
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
    } else {
	return nullptr;
    }
} 

} // namespace abc