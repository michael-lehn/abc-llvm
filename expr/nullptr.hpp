#ifndef EXPR_NULLPTRLITERAL_HPP
#define EXPR_NULLPTRLITERAL_HPP

#include <cstdint>

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class Nullptr : public Expr
{
    protected:
	Nullptr(lexer::Loc loc);

    public:
	static ExprPtr create(lexer::Loc loc = lexer::Loc{});

	// for sematic checks
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
	void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc
 
#endif // EXPR_NULLPTRLITERAL_HPP
