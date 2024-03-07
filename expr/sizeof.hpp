#ifndef EXPR_SIZEOF_HPP
#define EXPR_SIZEOF_HPP

#include <cstdint>

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class Sizeof : public Expr
{
    protected:
	Sizeof(const Type *sizeofType, ExprPtr &&sizeofExpr, lexer::Loc loc);

    public:
	static ExprPtr create(const Type *sizeofType,
			      lexer::Loc loc = lexer::Loc{});
	static ExprPtr create(ExprPtr &&sizeofExpr,
			      lexer::Loc loc = lexer::Loc{});

	const Type *sizeofType;
	const ExprPtr sizeofExpr;

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
 
#endif // EXPR_SIZEOF_HPP
