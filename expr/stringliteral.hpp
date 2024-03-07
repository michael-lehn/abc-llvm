#ifndef EXPR_STRINGLITERAL
#define EXPR_STRINGLITERAL

#include <cstddef>

#include "expr.hpp"

namespace abc {

class StringLiteral : public Expr
{
    protected:
	StringLiteral(const Type *type, UStr val, UStr valRaw, lexer::Loc loc);

    public:
	static ExprPtr create(UStr val, UStr valRaw,
			      lexer::Loc loc = lexer::Loc{});

	UStr		    val, valRaw;

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

#endif // EXPR_STRINGLITERAL
