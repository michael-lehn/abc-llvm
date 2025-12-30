#ifndef EXPR_EXPRLIST_HPP
#define EXPR_EXPRLIST_HPP

#include <vector>

#include "expr.hpp"

namespace abc {

class ExprList : public Expr
{
    public:
    protected:
	ExprList(std::vector<ExprPtr> &&exprVec);

    public:
	static ExprPtr create(std::vector<ExprPtr> &&exprVec);
	const std::vector<ExprPtr> exprVec;

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

#endif // EXPR_EXPRLIST_HPP
