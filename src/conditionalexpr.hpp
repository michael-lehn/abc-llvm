#ifndef CONDITIONALEXPR_HPP
#define CONDITIONALEXPR_HPP

#include "expr.hpp"

class ConditionalExpr : public Expr
{
    protected:
	ConditionalExpr(ExprPtr &&cond, ExprPtr &&thenExpr, ExprPtr &&elseExpr,
			const Type *type, Token::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&cond, ExprPtr &&thenExpr,
			      ExprPtr &&elseExpr,
			      Token::Loc loc = Token::Loc{});

	ExprPtr cond, thenExpr, elseExpr;

	bool hasAddr() const override;
	bool isLValue() const override;
	bool isConst() const override;

	// for code generation
	gen::ConstVal loadConstValue() const override;
	gen::Reg loadValue() const override;
	gen::Reg loadAddr() const override;
	void condJmp(gen::Label trueLabel,
			     gen::Label falseLabel) const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, bool isFactor) const override;
};

#endif // CONDITIONALEXPR_HPP
