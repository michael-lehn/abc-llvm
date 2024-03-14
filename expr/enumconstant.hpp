#ifndef EXPR_ENUMCONSTANT_HPP
#define EXPR_ENUMCONSTANT_HPP

#include <cstdint>

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class EnumConstant : public Expr
{
    protected:
	EnumConstant(UStr name, std::int64_t value, const Type *type,
		     lexer::Loc loc);

    public:
	static ExprPtr create(UStr name, std::int64_t value, const Type *type,
			      lexer::Loc loc);		

	const UStr name;
	const std::int64_t value;

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
 
#endif // EXPR_ENUMCONSTANT_HPP
