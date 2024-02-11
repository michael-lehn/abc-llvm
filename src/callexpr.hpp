#ifndef CALLEXPR_HPP
#define CALLEXPR_HPP

#include "expr.hpp"

class CallExpr : public Expr
{
    protected:
	CallExpr(ExprPtr &&fn, std::vector<ExprPtr> &&param, const Type *type,
		 Token::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&fn, std::vector<ExprPtr> &&param,
			      Token::Loc loc = Token::Loc{});

	ExprPtr fn;
	std::vector<ExprPtr> param;

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
};

#endif // CALLEXPR_HPP
