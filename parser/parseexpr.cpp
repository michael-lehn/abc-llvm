#include "expr/assertexpr.hpp"
#include "expr/binaryexpr.hpp"
#include "expr/callexpr.hpp"
#include "expr/characterliteral.hpp"
#include "expr/compoundexpr.hpp"
#include "expr/conditionalexpr.hpp"
#include "expr/enumconstant.hpp"
#include "expr/explicitcast.hpp"
#include "expr/identifier.hpp"
#include "expr/integerliteral.hpp"
#include "expr/member.hpp"
#include "expr/nullptr.hpp"
#include "expr/sizeof.hpp"
#include "expr/stringliteral.hpp"
#include "expr/unaryexpr.hpp"
#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "symtab/symtab.hpp"
#include "type/integertype.hpp"
#include "type/type.hpp"

#include "parseexpr.hpp"
#include "parser.hpp"

namespace abc {

using namespace lexer;

//------------------------------------------------------------------------------

ExprPtr
parseCompoundExpression(const Type *type)
{
    assert(type);

    auto tok = token;
    std::vector<ExprPtr> exprVec;
    if (type->isArray() && token.kind == TokenKind::STRING_LITERAL) {
	// string literals are treated as compound expression. For example:
	// "abc" is treated as {'a', 'b', 'c'}
	std::string str{};
	do {
	    str += token.processedVal.c_str();
	    getToken();
	} while (token.kind == TokenKind::STRING_LITERAL);
	for (char &c : str) {
	    auto val = UStr::create(std::string{c});
	    exprVec.push_back(CharacterLiteral::create(val, val, tok.loc));
	}
	return CompoundExpr::create(std::move(exprVec), type, tok.loc);
    } else if (token.kind == TokenKind::LBRACE) {
	getToken();
	for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	    auto ty = type->aggregateType(i);

	    // note: parseCompoundExpression has to be called before
	    //	 parseExpression. Because parseCompoundExpression catches
	    //	 string literals and treats them as a compound.
	    if (auto expr = parseCompoundExpression(ty)) {
		exprVec.push_back(std::move(expr));
	    } else if (auto expr = parseExpression()) {
		exprVec.push_back(std::move(expr));
	    } else {
		break;
	    }
	    if (token.kind != TokenKind::COMMA) {
		break;
	    }
	    getToken();
	}
	error::expected(TokenKind::RBRACE);
	getToken();
	return CompoundExpr::create(std::move(exprVec), type, tok.loc);
    } else {
	return nullptr;
    }
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

static ExprPtr
parseConditional()
{
    auto expr = parseBinary(1);
    if (!expr) {
	return nullptr;
    }

    if (token.kind == TokenKind::THEN || token.kind == TokenKind::QUERY) {
	auto tok = token;
	auto thenElseStyle = tok.kind == TokenKind::THEN;
	getToken();
	
	auto left = parseAssignment();
	if (!left) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	if (thenElseStyle) {
	    error::expected(TokenKind::ELSE);
	} else {
	    error::expected(TokenKind::COLON);
	}
	getToken();
	auto right = parseConditional();
	if (!right) {
	    error::out() << token.loc << " expected non-empty expression"
		<< std::endl;
	    error::fatal();
	}
	expr = ConditionalExpr::create(std::move(expr), std::move(left),
				       std::move(right), thenElseStyle,
				       tok.loc);
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
		error::location(token.loc);
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
    auto tok = token;
    switch (token.kind) {
	case TokenKind::LPAREN:
	    // Is it a C cast ?
	    getToken();
	    if (auto type = parseType()) {
		// Yes, it's a C cast!
		if (!error::expected(TokenKind::RPAREN)) {
		    return nullptr;
		}
		getToken();
		if (auto expr = parseCompoundExpression(type)) {
		    auto comp = dynamic_cast<const CompoundExpr *>(expr.get());
		    assert(comp);
		    comp->setDisplayOpt(CompoundExpr::PAREN);
		    return expr;
		} else {
		    return ExplicitCast::create(parsePrefix(), type, tok.loc);
		}
	    } else {
		// Ups, it was the '(' of a primary expression
		auto expr = parseExpression();
		if (!error::expected(TokenKind::RPAREN)) {
		    return nullptr;
		}
		getToken();
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::AND:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::ADDRESS,
				     parsePrefix(),
				     tok.loc);
	case TokenKind::ASTERISK:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::ASTERISK_DEREF,
				     parsePrefix(),
				     tok.loc);
	case TokenKind::PLUS2:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::PREFIX_INC,
				     parsePrefix(),
				     tok.loc);
	case TokenKind::MINUS2:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::PREFIX_DEC,
				     parsePrefix(),
				     tok.loc);
	case TokenKind::MINUS:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::MINUS,
				     parsePrefix(),
				     tok.loc);
	case TokenKind::NOT:
	    getToken();
	    return UnaryExpr::create(UnaryExpr::LOGICAL_NOT,
				     parsePrefix(),
				     tok.loc);
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

    if (!expr) {
	error::out() << token.loc
	    << ": postfix operator '" << token.kind << "' requires"
	    << " non-empty expression" << std::endl;
	error::fatal();

    }

    auto tok = token;
    switch (tok.kind) {
	case TokenKind::DOT:
	    {
		getToken();
		error::expected(TokenKind::IDENTIFIER);
		expr = Member::create(std::move(expr), token.val, token.loc);
		getToken();
		return parsePostfix(std::move(expr));
	    }
	case TokenKind::LPAREN:
	    getToken();
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
	    getToken();
	    if (auto index = parseExpression()) {
		error::expected(TokenKind::RBRACKET);
		getToken();
		expr = BinaryExpr::create(BinaryExpr::INDEX, std::move(expr),
					  std::move(index), tok.loc);
		return parsePostfix(std::move(expr));
	    } else {
		error::out() << token.loc << " expected non-empty expression"
		    << std::endl;
		error::fatal();
		return nullptr;
	    }
	    assert(0 && "Not implemented");
	    return nullptr;
	case TokenKind::ARROW:
	    getToken();
	    if (token.kind == TokenKind::IDENTIFIER) {
		auto tok = token;
		getToken();
		return  parsePostfix(Member::create(std::move(expr),
						    tok.val,
						    tok.loc));
	    } else {
		return parsePostfix(UnaryExpr::create(UnaryExpr::ARROW_DEREF,
						      std::move(expr),
						      tok.loc));
	    }
	case TokenKind::PLUS2:
	    getToken();
	    return parsePostfix(UnaryExpr::create(UnaryExpr::POSTFIX_INC,
						  std::move(expr),
						  tok.loc));
	case TokenKind::MINUS2:
	    getToken();
	    return parsePostfix(UnaryExpr::create(UnaryExpr::POSTFIX_DEC,
						  std::move(expr),
						  tok.loc));
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
	if (auto sym = Symtab::type(tok.val, Symtab::AnyScope)) {
	    tok = token;
	    assert(sym->type);
	    auto expr = parseCompoundExpression(sym->type);
	    if (!expr) {
		error::out() << tok.loc << ": expected compound expression "
		    << "after type " << sym->type << "\n";
		error::fatal();
		return nullptr;
	    } else {
		auto comp = dynamic_cast<const CompoundExpr *>(expr.get());
		assert(comp);
		comp->setDisplayOpt(CompoundExpr::BRACE);
		return expr;
	    }
	} else if (auto sym = Symtab::constant(tok.val, Symtab::AnyScope)) {
	    auto expr = EnumConstant::create(tok.val,
					     sym->expr->getSignedIntValue(),
					     sym->expr->type,
					     tok.loc);
	    return expr;
	} else if (auto sym = Symtab::variable(tok.val, Symtab::AnyScope)) {
	    auto ty = sym->type;
	    auto expr = Identifier::create(tok.val, sym->id, ty, tok.loc);
	    return expr;
	} else {
	    error::location(tok.loc);
	    error::out() << tok.loc << ": error undefined identifier\n";
	    error::fatal();
	    return nullptr;
	}
    } else if (tok.kind == TokenKind::DECIMAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.processedVal, 10, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::HEXADECIMAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.processedVal, 16, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::OCTAL_LITERAL) {
	getToken();
	auto ty = parseIntType(); // parse suffix
	auto expr = IntegerLiteral::create(tok.processedVal, 8, ty, tok.loc);
        return expr;
    } else if (token.kind == TokenKind::CHARACTER_LITERAL) {
	getToken();
	return CharacterLiteral::create(tok.processedVal, tok.val, tok.loc);
    } else if (token.kind == TokenKind::STRING_LITERAL) {
	std::string str{};
	std::string strRaw{};
	do {
	    str += token.processedVal.c_str();
	    strRaw += token.val.c_str();
	    getToken();
	} while (token.kind == TokenKind::STRING_LITERAL);
	auto val = UStr::create(str);
	auto valRaw = UStr::create(strRaw);
	return StringLiteral::create(val, valRaw, tok.loc);
    } else if (token.kind == TokenKind::NULLPTR) {
        getToken();
        return Nullptr::create(tok.loc);
    } else if (token.kind == TokenKind::SIZEOF) {
	getToken();
	if (!error::expected(TokenKind::LPAREN)) {
	    return nullptr;
	}
	getToken();
	auto tok = token;
	ExprPtr sizeofExpr;
	if (auto type = parseType()) {
	    sizeofExpr = Sizeof::create(type, tok.loc);
	} else if (auto expr = parseExpression()) {
	    sizeofExpr = Sizeof::create(std::move(expr), tok.loc);
	} else {
	    error::out() << token.loc << " expected type or expression\n";
	    error::fatal();
	    return nullptr;
	}
	if (!error::expected(TokenKind::RPAREN)) {
	    return nullptr;
	}
	getToken();
	return sizeofExpr;
    } else if (token.kind == TokenKind::ASSERT) {
	getToken();
	auto expr = parseExpression();
	if (!expr) {
	    error::out() << token.loc << " expected non-empty expression\n";
	    error::fatal();
	}
	return AssertExpr::create(std::move(expr), tok.loc);
    } else if (token.kind == TokenKind::LPAREN) {
        getToken();
	auto expr = parseExpression();
        if (!expr) {
	    error::out() << token.loc << " expected non-empty expression\n";
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
