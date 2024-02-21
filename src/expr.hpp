#ifndef EXPR_HPP
#define EXPR_HPP

#include <cstdint>
#include <memory>

#include "gen.hpp"
#include "type.hpp"
#include "lexer.hpp"

class Expr
{
    protected:
	Expr(Token::Loc  loc, const Type  *type);

    public:
	virtual ~Expr() = default;

    public:
	const Token::Loc    loc;
	const Type	    * const type;

    public:
	// for sematic checks
	virtual bool hasAddr() const = 0;
	virtual bool isLValue() const = 0;
	virtual bool isConst() const = 0;

	// get value from const expressions
	gen::ConstIntVal getConstIntValue() const;
	std::int64_t getSignedIntValue() const;
	std::uint64_t getUnsignedIntValue() const;

	// for code generation
	virtual gen::ConstVal loadConstValue() const = 0;
	virtual gen::Reg loadValue() const = 0;
	virtual gen::Reg loadAddr() const = 0;
	virtual void condJmp(gen::Label trueLabel,
			     gen::Label falseLabel) const = 0;

	// for debugging and educational purposes
	virtual void print(int indent = 1) const = 0;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const = 0;
};

using ExprPtr = std::unique_ptr<const Expr>;

std::ostream &operator<<(std::ostream &out, const ExprPtr &expr);
std::ostream &operator<<(std::ostream &out, const Expr *expr);


#endif // EXPR_HPP
