#ifndef EXPR_CHARACTERLITERAL_HPP
#define EXPR_CHARACTERLITERAL_HPP

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class CharacterLiteral : public Expr
{
    protected:
	CharacterLiteral(unsigned processedVal, UStr val, lexer::Loc loc);

    public:
	static ExprPtr create(UStr processedVal, UStr val,
	                      lexer::Loc loc = lexer::Loc{});

	const unsigned processedVal;
	const UStr val;

	// for sematic checks
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
	void printFlat(std::ostream &out, int prec) const override;
};

} // namespace abc

#endif // EXPR_CHARACTERLITERAL_HPP
