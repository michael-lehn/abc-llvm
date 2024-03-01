#ifndef IDENTIFIER_HPP
#define IDENTIFIER_HPP

#include "expr.hpp"
#include "lexer/loc.hpp"

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

	bool hasAddr() const override;
	bool isLValue() const override;
	bool isConst() const override;

	// for code generation
	gen::Constant loadConstant() const override;
	gen::Value loadValue() const override;
	gen::Value loadAddress() const override;
	void condJmp(gen::Label trueLabel,
		     gen::Label falseLabel) const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc

#endif // IDENTIFIER_HPP
