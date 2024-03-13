#ifndef EXPR_COMPOUNDEXPR_HPP
#define EXPR_COMPOUNDEXPR_HPP

#include <vector>

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class CompoundExpr : public Expr
{
    protected:
	CompoundExpr(std::vector<ExprPtr> &&exprVec, const Type *type,
		     lexer::Loc loc);

	UStr tmpId;

	void initTmp() const;

    public:
	static ExprPtr create(std::vector<ExprPtr> &&exprVec, const Type *type,
			      lexer::Loc loc = lexer::Loc{});

	const std::vector<ExprPtr> exprVec;

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

#endif // EXPR_COMPOUNDEXPR_HPP
