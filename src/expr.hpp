#ifndef EXPR_HPP
#define EXPR_HPP

#include <cstdint>
#include <memory>
#include <string>

#include "gen.hpp"
#include "type.hpp"

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

    Literal(const char *val, const Type *type, std::uint8_t radix)
	: val{val}, type{type}, radix{radix} {}
};

struct Identifier
{
    const char *val;
    const Type *type;

    Identifier(const char *val, const Type *type) : val{val}, type{type} {}
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

    Unary(Kind kind, ExprPtr &&child, const Type *type)
	: kind{kind}, child{std::move(child)}, type{type}
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
    };

    Kind kind;
    ExprPtr left, right;
    const Type *type;

    Binary(Kind kind, ExprPtr &&left, ExprPtr &&right)
	: kind{kind}, left{std::move(left)}, right{std::move(right)},
	  type{nullptr}
    {
	setTypeAndCastOperands();
    }

    void setTypeAndCastOperands(void);
};

struct Conditional
{
    ExprPtr cond, left, right;
    const Type *type;

    Conditional(ExprPtr &&cond, ExprPtr &&left, ExprPtr &&right)
	: cond{std::move(cond)}, left{std::move(left)}, right{std::move(right)}
    {
	setTypeAndCastOperands();
    }

    void setTypeAndCastOperands(void);
};

class Expr
{
    public:
	std::variant<Literal, Identifier, Unary, Binary, Conditional,
		     ExprVector> variant;

    private:
	Expr(Literal &&val) : variant{std::move(val)} {}
	Expr(Identifier &&ident) : variant{std::move(ident)} {}
	Expr(Unary &&unary) : variant{std::move(unary)} {}
	Expr(Binary &&binary) : variant{std::move(binary)} {}
	Expr(Conditional &&con) : variant{std::move(con)} {}
	Expr(ExprVector &&vec) : variant{std::move(vec)} {}

    public:
	static ExprPtr createLiteral(const char *val, std::uint8_t radix,
				     const Type *type = nullptr);
	static ExprPtr createIdentifier(const char *ident, const Type *type);
	static ExprPtr createUnaryMinus(ExprPtr &&expr);
	static ExprPtr createLogicalNot(ExprPtr &&expr);
	static ExprPtr createCast(ExprPtr &&child, const Type *toType);
	static ExprPtr createBinary(Binary::Kind kind,
				    ExprPtr &&left, ExprPtr &&right);
	static ExprPtr createCall(ExprPtr &&fn, ExprVector &&param);
	static ExprPtr createConditional(ExprPtr &&cond,
					 ExprPtr &&left, ExprPtr &&right);
	static ExprPtr createExprVector(ExprVector &&expr);

	void print(int indent = 0) const;
	const Type *getType(void) const;
	bool isLValue(void) const;
	bool isConst(void) const;

	// code generation
	gen::ConstVal loadConst(void) const;
	gen::Reg loadValue(void) const;
	void condJmp(gen::Label trueLabel, gen::Label falseLabel) const;
};


#endif // EXPR_HPP
