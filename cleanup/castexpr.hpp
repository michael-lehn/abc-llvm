#ifndef CASTEXPR_HPP
#define CASTEXPR_HPP

#include "expr.hpp"

class CastExpr : public Expr
{
    protected:
	CastExpr(ExprPtr &&expr, const Type *toType, Token::Loc loc,
		 bool allowConstCast);

    public:

	// 'allowConstCast' actually means 'explicit cast'
	static ExprPtr create(ExprPtr &&expr, const Type *toType,
			      Token::Loc loc, bool allowConstCast = false);

	static ExprPtr create(ExprPtr &&expr, const Type *toType);

	bool explicitCast;
	const ExprPtr expr;

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
	virtual void printFlat(std::ostream &out, int prec) const override;
};

#endif // CASTEXPR_HPP
