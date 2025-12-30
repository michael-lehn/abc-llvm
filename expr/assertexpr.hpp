#ifndef EXPR_ASSERTEXPR_HPP
#define EXPR_ASSERTEXPR_HPP

#include "expr.hpp"

namespace abc {

class AssertExpr : public Expr
{
    protected:
	AssertExpr(ExprPtr &&expr, lexer::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&expr, lexer::Loc loc = lexer::Loc{});
	static void setFunction(UStr name, const Type *fnType);

	ExprPtr expr;

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

#endif // EXPR_ASSERTEXPR_HPP
