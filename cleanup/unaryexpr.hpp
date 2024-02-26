#ifndef UNARYEXPR_HPP
#define UNARYEXPR_HPP

#include "expr.hpp"

class UnaryExpr : public Expr
{
   public:
	enum Kind
	{
	    ADDRESS,
	    DEREF,
	    LOGICAL_NOT,
	    POSTFIX_INC,
	    POSTFIX_DEC,
	    MINUS,
	};

    protected:
	UnaryExpr(Kind kind, ExprPtr &&child, const Type *type, Token::Loc loc);

    public:
	static ExprPtr create(Kind kind, ExprPtr &&child,
			      Token::Loc loc = Token::Loc{});

	const Kind kind;
	const ExprPtr child;

    public:
	bool hasAddr() const override;
	bool isLValue() const override;
	bool isIntegerConstExpr() const;
	bool isArithmeticConstExpr() const;
	bool isAddressConstant() const;
	bool isConst() const override;

	// for code generation
	gen::ConstVal loadConstValue() const override;
	gen::Reg loadValue() const override;
	gen::Reg loadAddr() const override;
	void condJmp(gen::Label trueLabel,
		     gen::Label falseLabel) const override;

	// for debugging and educational purposes
	void print(int indent) const override;

	// for printing error messages
	virtual void printFlat(std::ostream &out, int prec) const override;
};


#endif // UNARYEXPR_HPP
