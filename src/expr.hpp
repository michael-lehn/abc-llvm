#ifndef EXPR_HPP
#define EXPR_HPP

#include <charconv>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include "error.hpp"
#include "gen.hpp"
#include "lexer.hpp"
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

struct IntegerLiteral
{
    UStr val;
    const Type *type;
    std::uint8_t radix;
    Token::Loc loc;

    IntegerLiteral(UStr val, const Type *type, std::uint8_t radix,
		   Token::Loc loc);
};

struct StringLiteral
{
    UStr val;
    std::variant<UStr, std::size_t> data;
    const Type *type;
    Token::Loc loc;

    StringLiteral(UStr val, UStr ident, Token::Loc loc);
    StringLiteral(UStr val, std::size_t zeroPadding, Token::Loc loc);
};


struct Identifier
{
    UStr ident;
    const Type *type;
    Token::Loc loc;

    Identifier(UStr ident, const Type *type, Token::Loc loc);
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

    // Type explicitly given (e.g. cast)
    Unary(Kind kind, ExprPtr &&child, const Type *type, Token::Loc opLoc)
	: kind{kind}, child{std::move(child)}, type{type}, opLoc{opLoc}
    {
    }

    // Type implicitly specified by operation (e.g. logical not)
    Unary(Kind kind, ExprPtr &&child, Token::Loc opLoc)
	: kind{kind}, child{std::move(child)}, type{nullptr}, opLoc{opLoc}
    {
	setTypeAndCastOperands();
    }

    void setTypeAndCastOperands(void);
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
	MEMBER,
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
	std::variant<IntegerLiteral, StringLiteral, Identifier, Proxy,  Unary,
		     Binary, Conditional, ExprVector> variant;

    private:
	Expr(IntegerLiteral &&val) : variant{std::move(val)} {}
	Expr(StringLiteral &&val) : variant{std::move(val)} {}
	Expr(Identifier &&ident) : variant{std::move(ident)} {}
	Expr(Proxy &&proxy) : variant{std::move(proxy)} {}
	Expr(Unary &&unary) : variant{std::move(unary)} {}
	Expr(Binary &&binary) : variant{std::move(binary)} {}
	Expr(Conditional &&con) : variant{std::move(con)} {}
	Expr(ExprVector &&vec) : variant{std::move(vec)} {}


    public:
	static ExprPtr createNull(const Type *type,
				  Token::Loc loc = Token::Loc{});
	static ExprPtr createIntegerLiteral(UStr val, std::uint8_t radix = 10,
					    const Type *type = nullptr,
					    Token::Loc loc = Token::Loc{});
	static ExprPtr createStringLiteral(UStr val, UStr ident,
					   Token::Loc loc = Token::Loc{});
    private:
	static ExprPtr createStringLiteral(UStr val, std::size_t zeroPadding,
					   Token::Loc loc = Token::Loc{});
    public:
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
	static ExprPtr createMember(ExprPtr &&from, UStr memberIdent,
				    Token::Loc opLoc = Token::Loc{});
	static ExprPtr createCall(ExprPtr &&fn, ExprVector &&param,
				  Token::Loc opLoc = Token::Loc{});
	static ExprPtr createConditional(ExprPtr &&cond,
					 ExprPtr &&left, ExprPtr &&right,
					 Token::Loc opLeftLoc = Token::Loc{},
					 Token::Loc opRightLoc = Token::Loc{});
	static ExprPtr createExprVector(ExprVector &&expr);

	const Type *getType(void) const;
	bool hasAddr(void) const;
	bool isLValue(void) const;
	bool isConst(void) const;
	Token::Loc getLoc(void) const;

	void print(int indent = 0) const;

	// code generation
	gen::ConstVal loadConst(void) const;
	gen::ConstIntVal loadConstInt(void) const;
	gen::Reg loadValue(void) const;
	gen::Reg loadAddr(void) const;
	void condJmp(gen::Label trueLabel, gen::Label falseLabel) const;
};

#endif // EXPR_HPP
