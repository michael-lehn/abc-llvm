#ifndef EXPR_IMPLICITCAST_HPP
#define EXPR_IMPLICITCAST_HPP

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class ImplicitCast : public Expr
{
    protected:
	ImplicitCast(ExprPtr &&expr, const Type *toType, lexer::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&expr, const Type *toType);
	static bool setOutput(bool on);
	const ExprPtr expr;

	void apply(std::function<bool(const Expr *)> op) const override;
	bool hasAddress() const override;
	bool isLValue() const override;
	bool isConst() const override;

	// for code generation
	gen::Constant loadConstant() const override;
	gen::Value loadValue() const override;
	gen::Value loadAddress() const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc

#endif // EXPR_IMPLICITCAST_HPP
