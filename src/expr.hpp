#ifndef EXPR_HPP
#define EXPR_HPP

#include <charconv>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "error.hpp"
#include "gen.hpp"
#include "lexer.hpp"
#include "symtab.hpp"
#include "type.hpp"
#include "ustr.hpp"

class Expr;
struct ExprDeleter
{
    void
    operator()(const Expr *expr) const;
};

using ExprPtr = std::unique_ptr<const Expr, ExprDeleter>;
using ExprVector = std::vector<ExprPtr>;
using ExprVectorPtr = std::unique_ptr<ExprVector>;

struct Literal
{
    const char *val;
    const Type *type;
    std::uint8_t radix;
    Token::Loc loc;

    Literal(const char *val, const Type *type, std::uint8_t radix,
	    Token::Loc loc)
	: val{val}, type{type}, radix{radix}, loc{loc}
    {}
};

struct Identifier
{
    const char *val;
    const Type *type;
    Token::Loc loc;

    Identifier(UStr ident, Token::Loc loc)
	: loc{loc}
    {
	auto symEntry = symtab::get(ident);
	if (symEntry) {
	    val = symEntry->internalIdent.c_str();
	    type = symEntry->type;
	    return;
	}
	error::out() << loc << " undeclared identifier '"
	    << ident.c_str() << "'" << std::endl;
	error::fatal(); // TODO: use error::raise()
    }
};

struct Proxy
{
    const Expr *expr;

    Proxy(const Expr *expr) : expr{expr} {}
};

struct Unary
{
    enum Kind
    {
	ADDRESS,
	DEREF,
	CAST,
	LOGICAL_NOT,
    };

    Kind kind;
    ExprPtr child;
    const Type *type;
    Token::Loc opLoc; // location of operator

    Unary(Kind kind, ExprPtr &&child, const Type *type, Token::Loc opLoc)
	: kind{kind}, child{std::move(child)}, type{type}, opLoc{opLoc}
    {}

    const Type * getType(void);
};

struct Binary
{
    enum Kind
    {
	CALL,
	ADD,
	ASSIGN,
	EQUAL,
	NOT_EQUAL,
	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,
	LOGICAL_AND,
	LOGICAL_OR,
	SUB,
	MUL,
	DIV,
	MOD,
	POSTFIX_INC,
	POSTFIX_DEC,
    };

    Kind kind;
    ExprPtr left, right;
    const Type *type;
    Token::Loc opLoc;

    Binary(Kind kind, ExprPtr &&left, ExprPtr &&right, Token::Loc opLoc)
	: kind{kind}, left{std::move(left)}, right{std::move(right)}
	, type{nullptr}, opLoc{opLoc}
    {
	setType();
	//assert(type && "illegal expression"); // TODO: better error handling
	castOperands();
    }

    void setType(void);
    void castOperands(void);
    
};

struct Conditional
{
    ExprPtr cond, left, right;
    const Type *type;
    Token::Loc opLeftLoc, opRightLoc;

    Conditional(ExprPtr &&cond, ExprPtr &&left, ExprPtr &&right,
	        Token::Loc opLeftLoc, Token::Loc opRightLoc)
	: cond{std::move(cond)}, left{std::move(left)}, right{std::move(right)}
	, opLeftLoc{opLeftLoc}, opRightLoc{opRightLoc}

    {
	setTypeAndCastOperands();
    }

    void setTypeAndCastOperands(void);
};

class Expr
{
    public:
	std::variant<Literal, Identifier, Proxy,  Unary, Binary, Conditional,
		     ExprVector> variant;

    private:
	Expr(Literal &&val) : variant{std::move(val)} {}
	Expr(Identifier &&ident) : variant{std::move(ident)} {}
	Expr(Proxy &&proxy) : variant{std::move(proxy)} {}
	Expr(Unary &&unary) : variant{std::move(unary)} {}
	Expr(Binary &&binary) : variant{std::move(binary)} {}
	Expr(Conditional &&con) : variant{std::move(con)} {}
	Expr(ExprVector &&vec) : variant{std::move(vec)} {}

    public:
	static ExprPtr createLiteral(const char *val, std::uint8_t radix,
				     const Type *type = nullptr,
				     Token::Loc loc = Token::Loc{});
	static ExprPtr createIdentifier(UStr ident,
					Token::Loc loc = Token::Loc{});
	static ExprPtr createProxy(const Expr *expr);
	static ExprPtr createUnaryMinus(ExprPtr &&expr,
				        Token::Loc opLoc = Token::Loc{});
	static ExprPtr createLogicalNot(ExprPtr &&expr,
					Token::Loc opLoc = Token::Loc{});
	static ExprPtr createAddr(ExprPtr &&expr,
				  Token::Loc opLoc = Token::Loc{});
	static ExprPtr createDeref(ExprPtr &&expr,
				   Token::Loc opLoc = Token::Loc{});
	static ExprPtr createCast(ExprPtr &&child, const Type *toType,
				  Token::Loc opLoc = Token::Loc{});
	static ExprPtr createBinary(Binary::Kind kind,
				    ExprPtr &&left, ExprPtr &&right,
				    Token::Loc opLoc = Token::Loc{});
	static ExprPtr createCall(ExprPtr &&fn, ExprVector &&param,
				  Token::Loc opLoc = Token::Loc{});
	static ExprPtr createConditional(ExprPtr &&cond,
					 ExprPtr &&left, ExprPtr &&right,
					 Token::Loc opLeftLoc = Token::Loc{},
					 Token::Loc opRightLoc = Token::Loc{});
	static ExprPtr createExprVector(ExprVector &&expr);

	const Type *getType(void) const;
	bool isLValue(void) const;
	bool isConst(void) const;

	void print(int indent = 0) const;

	template <typename T>
	T constValue(void) const;

	// code generation
	gen::ConstVal loadConst(void) const;
	gen::Reg loadValue(void) const;
	gen::Reg loadAddr(void) const;
	void condJmp(gen::Label trueLabel, gen::Label falseLabel) const;
};

template <typename T>
T
Expr::constValue(void) const
{
    assert(isConst());

    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->constValue<T>();
    } else if (std::holds_alternative<Literal>(variant)) {
	const auto &lit = std::get<Literal>(variant);
	T result;
	auto [ptr, ec] = std::from_chars(
			    lit.val, lit.val + strlen(lit.val),
			    result, lit.radix);
	assert(ec == std::errc{});
	return result;
    } else if (std::holds_alternative<Unary>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<Binary>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<Conditional>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<ExprVector>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else {
	std::cerr << "not handled variant. Index = " << variant.index()
	    << std::endl;
	assert(0);
	return 0;
    }
}

#endif // EXPR_HPP
