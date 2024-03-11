#ifndef EXPR_IDENTIFIER_HPP
#define EXPR_IDENTIFIER_HPP

#include "type/type.hpp"
#include "lexer/loc.hpp"

#include "expr.hpp"

namespace abc {

class Identifier : public Expr
{
    protected:
	Identifier(UStr name, UStr id, const Type *type, lexer::Loc loc);

    public:
	static ExprPtr create(UStr name, UStr id, const Type *type,
			      lexer::Loc loc = lexer::Loc{});

	const UStr name;
	const UStr id;

	virtual bool hasConstantAddress() const override;
	bool hasAddress() const override;
	bool isLValue() const override;
	bool isConst() const override;

	// for code generation
	gen::Constant loadConstant() const override;
	gen::Value loadValue() const override;
	gen::Constant loadConstantAddress() const override;
	gen::Value loadAddress() const override;
	void condition(gen::Label trueLabel,
		       gen::Label falseLabel) const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc

#endif // EXPR_IDENTIFIER_HPP
