#ifndef EXPR_CALLEXPR_HPP
#define EXPR_CALLEXPR_HPP

#include "lexer/token.hpp"

#include "expr.hpp"

namespace abc {

class CallExpr : public Expr
{
    protected:
	CallExpr(ExprPtr &&fn, std::vector<ExprPtr> &&arg, const Type *type,
		 lexer::Loc loc);

	UStr tmpId;

	void initTmp() const;

    public:
	static ExprPtr create(ExprPtr &&fn, std::vector<ExprPtr> &&arg,
			      lexer::Loc loc = lexer::Loc{});

	ExprPtr fn;
	std::vector<ExprPtr> arg;

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

#endif // EXPR_CALLEXPR_HPP
