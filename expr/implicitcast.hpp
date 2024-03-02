#ifndef IMPLICITCAST_HPP
#define IMPLICITCAST_HPP

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class ImplicitCast : public Expr
{
    protected:
	ImplicitCast(ExprPtr &&expr, const Type *toType, lexer::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&expr, const Type *toType, 
			      lexer::Loc loc = lexer::Loc{});
	const ExprPtr expr;

	bool hasAddress() const override;
	bool isLValue() const override;
	bool isConst() const override;

	// for code generation
	gen::Constant loadConstant() const override;
	gen::Value loadValue() const override;
	gen::Value loadAddress() const override;
	void condition(gen::Label trueLabel,
		       gen::Label falseLabel) const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc

#endif // IMPLICITCAST_HPP
