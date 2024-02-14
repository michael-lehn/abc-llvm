#ifndef EXPRVECTOR_HPP
#define EXPRVECTOR_HPP

#include "expr.hpp"

class ExprVector : public Expr
{
    protected:
	ExprVector(std::vector<ExprPtr> &&exprVec, const Type *type,
		   Token::Loc loc);

    public:
	static ExprPtr create(std::vector<ExprPtr> &&exprVec,
			      Token::Loc loc = Token::Loc{});

	std::vector<ExprPtr> exprVec;

	bool hasAddr() const override;
	bool isLValue() const override;
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
	virtual void printFlat(std::ostream &out, bool isFactor) const override;
};

#endif // EXPRVECTOR_HPP
