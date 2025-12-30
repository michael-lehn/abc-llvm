#ifndef EXPR_CONDITIONALEXPR_HPP
#define EXPR_CONDITIONALEXPR_HPP

#include "lexer/loc.hpp"
#include "type/type.hpp"

#include "expr.hpp"

namespace abc {

class ConditionalExpr : public Expr
{
    protected:
	ConditionalExpr(ExprPtr cond, ExprPtr trueExpr, ExprPtr falseExpr,
	                const Type *type, bool thenElseStyle, lexer::Loc loc);

    public:
	static ExprPtr create(ExprPtr cond, ExprPtr trueExpr, ExprPtr falseExpr,
	                      bool thenElseStyle = false,
	                      lexer::Loc loc = lexer::Loc{});

	const ExprPtr cond, trueExpr, falseExpr;
	bool thenElseStyle;

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

#endif // EXPR_CONDITIONALEXPR_HPP
