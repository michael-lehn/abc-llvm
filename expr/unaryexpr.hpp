#ifndef EXPR_UNARYEXPR_HPP
#define EXPR_UNARYEXPR_HPP

#include "expr.hpp"
#include "lexer/loc.hpp"

namespace abc {

class UnaryExpr : public Expr
{
   public:
	enum Kind
	{
	    ADDRESS,
	    ARROW_DEREF,
	    ASTERISK_DEREF,
	    LOGICAL_NOT,
	    MINUS,
	    POSTFIX_DEC,
	    POSTFIX_INC,
	    PREFIX_DEC,
	    PREFIX_INC,
	};

    protected:
	UnaryExpr(Kind kind, ExprPtr &&child, const Type *type, lexer::Loc loc);

    public:
	static ExprPtr create(Kind kind, ExprPtr &&child,
			      lexer::Loc loc = lexer::Loc{});

	const Kind kind;
	const ExprPtr child;

	bool hasAddress() const override;
	bool isLValue() const override;

    private:
	bool isIntegerConstExpr() const;
	bool isArithmeticConstExpr() const;
	bool isAddressConstant() const;

    public:
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

#endif // EXPR_UNARYEXPR_HPP
