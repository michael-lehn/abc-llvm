#ifndef EXPR_MEMBERACCESS_HPP
#define EXPR_MEMBERACCESS_HPP

#include "type/type.hpp"
#include "lexer/loc.hpp"

#include "expr.hpp"

namespace abc {

class Member : public Expr
{
    protected:
	Member(ExprPtr &&structure, UStr member, const Type *type,
	       std::size_t index, lexer::Loc loc);

    public:
	static ExprPtr create(ExprPtr &&structure, UStr member,
			      lexer::Loc loc = lexer::Loc{});

	const ExprPtr structure;
	const UStr member;
	const std::size_t index;

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

#endif // EXPR_MEMBERACCESS_HPP
