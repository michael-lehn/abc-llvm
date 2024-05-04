#ifndef EXPR_FLOATLITERAL_HPP
#define EXPR_FLOATLITERAL_HPP

#include <cstdint>

#include "expr.hpp"
#include "lexer/loc.hpp"
#include "type/floattype.hpp"

namespace abc {

class FloatLiteral : public Expr
{
    protected:
	FloatLiteral(UStr val, const Type *type, lexer::Loc loc);

    public:
	static ExprPtr create(UStr val,
			      const Type *type = FloatType::createDouble(),
			      lexer::Loc loc = lexer::Loc{});

	static ExprPtr create(double val,
			      const Type *type = FloatType::createDouble(),
			      lexer::Loc loc = lexer::Loc{});

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
 
#endif // EXPR_FLOATLITERAL_HPP
