#ifndef BINARYEXPR_HPP
#define BINARYEXPR_HPP

#include "expr.hpp"

class BinaryExpr : public Expr
{
   public:
	enum Kind
	{
	    CALL,
	    ADD,
	    ASSIGN,
	    EQUAL,
	    NOT_EQUAL,
	    GREATER,
	    GREATER_EQUAL,
	    LESS,
	    LESS_EQUAL,
	    LOGICAL_AND,
	    LOGICAL_OR,
	    SUB,
	    MUL,
	    DIV,
	    MOD,
	    MEMBER,
	};

    protected:
	BinaryExpr(Kind kind, ExprPtr &&left, ExprPtr &&right, const Type *type,
		   Token::Loc loc);

    public:
	static ExprPtr create(Kind kind, ExprPtr &&left, ExprPtr &&right,
			      Token::Loc loc = Token::Loc{});

	const Kind kind;
	const ExprPtr left, right;

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
  
};

#endif // BINARYEXPR_HPP
