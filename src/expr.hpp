#ifndef EXPR_HPP
#define EXPR_HPP

#include <string>
#include <memory>

#include "gen.hpp"

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

    Literal(const char *val) : val{val} {}
};

struct Identifier
{
    const char *val;

    Identifier(const char *val) : val{val} {}
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
    };

    Kind kind;
    ExprPtr left, right;

    Binary(Kind kind, ExprPtr &&left, ExprPtr &&right)
	: kind{kind}, left{std::move(left)}, right{std::move(right)}
    {}
};

class Expr
{
    public:
	std::variant<Literal, Identifier, Binary, ExprVector> variant;

    private:

	Expr(Literal &&val) : variant{std::move(val)} {}
	Expr(Identifier &&ident) : variant{std::move(ident)} {}
	Expr(Binary &&binary) : variant{std::move(binary)} {}
	Expr(ExprVector &&vec) : variant{std::move(vec)} {}

    public:
	static ExprPtr getLiteral(const char *val);
	static ExprPtr getIdentifier(const char *ident);
	static ExprPtr getUnaryMinus(ExprPtr &&expr);
	static ExprPtr getBinary(Binary::Kind kind,
				 ExprPtr &&left, ExprPtr &&right);
	static ExprPtr getCall(ExprPtr &&fn, ExprVector &&param);
	static ExprPtr getExprVector(ExprVector &&expr);

	void print(int indent = 0) const;
	bool isConst(void) const;
	gen::ConstVal getConst(void) const;
	gen::Reg load(void) const;
	void condJmp(gen::Label trueLabel, gen::Label falseLabel) const;
};


#endif // EXPR_HPP
