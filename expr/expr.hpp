#ifndef EXPR_EXPR_HPP
#define EXPR_EXPR_HPP

#include <cstdint>
#include <memory>

#include "gen/gen.hpp"
#include "lexer/loc.hpp"
#include "type/type.hpp"

namespace abc {

class Expr
{
    protected:
	Expr(lexer::Loc loc, const Type *type);

    public:
	virtual ~Expr() = default;

	const lexer::Loc loc;
	const Type *const type;

	// for sematic checks
	virtual bool hasConstantAddress() const;
	virtual bool hasAddress() const = 0;
	virtual bool isLValue() const = 0;
	virtual bool isConst() const = 0;

	// for code generation
	virtual gen::Constant loadConstant() const = 0;
	virtual gen::Value loadValue() const = 0;
	virtual gen::Constant loadConstantAddress() const;
	virtual gen::Value loadAddress() const = 0;
	virtual void condition(gen::Label trueLabel,
	                       gen::Label falseLabel) const;

	// for debugging and educational purposes
	virtual void print(int indent = 1) const = 0;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const = 0;

	// get value from const expressions with integer type
	gen::ConstantInt getConstantInt() const;
	std::int64_t getSignedIntValue() const;
	std::uint64_t getUnsignedIntValue() const;
};

using ExprPtr = std::unique_ptr<const Expr>;

std::ostream &operator<<(std::ostream &out, const ExprPtr &expr);
std::ostream &operator<<(std::ostream &out, const Expr *expr);

} // namespace abc

#endif // EXPR_EXPR_HPP
