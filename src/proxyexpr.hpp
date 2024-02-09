#ifndef PROXYEXPR_HPP
#define PROXYEXPR_HPP

#include "expr.hpp"

class ProxyExpr : public Expr
{
    protected:
	ProxyExpr(const Expr *expr, Token::Loc loc);

    public:
	static ExprPtr create(const Expr *expr, Token::Loc loc = Token::Loc{});

	const Expr * const expr;

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

#endif // PROXYEXPR_HPP
