#ifndef EXPR_INTEGERLITERAL_HPP
#define EXPR_INTEGERLITERAL_HPP

#include <cstdint>

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class IntegerLiteral : public Expr
{
    protected:
	IntegerLiteral(UStr val, std::uint8_t  radix, const Type *type,
		       lexer::Loc loc);

    public:
	static ExprPtr create(UStr val, std::uint8_t  radix = 10,
			      const Type *type = nullptr,
			      lexer::Loc loc = lexer::Loc{});

	static ExprPtr create(std::int64_t val,
			      const Type *type = nullptr,
			      lexer::Loc loc = lexer::Loc{});

	const UStr val;
	const std::uint8_t radix;

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
 
#endif // EXPR_INTEGERLITERAL_HPP
