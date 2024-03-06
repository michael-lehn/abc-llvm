#ifndef BINARYEXPR_HPP
#define BINARYEXPR_HPP

#include "gen/gen.hpp"
#include "lexer/loc.hpp"

#include "expr.hpp"

namespace abc {

class BinaryExpr : public Expr
{
   public:
	enum Kind
	{
	    // assignments
	    ASSIGN,
	    ADD_ASSIGN,
	    SUB_ASSIGN,
	    MUL_ASSIGN,
	    DIV_ASSIGN,
	    MOD_ASSIGN,

	    // index operator
	    INDEX,

	    // arithmetic
	    ADD,
	    SUB,
	    MUL,
	    DIV,
	    MOD,

	    // logical arithmetic
	    EQUAL,
	    NOT_EQUAL,
	    GREATER,
	    GREATER_EQUAL,
	    LESS,
	    LESS_EQUAL,

	    // logical connective
	    LOGICAL_AND,
	    LOGICAL_OR,
	};

    protected:
	BinaryExpr(Kind kind, ExprPtr &&left, ExprPtr &&right, const Type *type,
		   lexer::Loc loc);

    public:
	static ExprPtr create(Kind kind, ExprPtr &&left, ExprPtr &&right,
			      lexer::Loc loc = lexer::Loc{});

	const Kind kind;
	const ExprPtr left, right;

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
    private:
	gen::Value handleArithmetricOperation(Kind kind) const;
    public:
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

#endif // BINARYEXPR_HPP
