#ifndef IDENTIFIER_HPP
#define IDENTIFIER_HPP

#include "expr.hpp"

class Identifier : public Expr
{
    protected:
	Identifier(UStr ident, const Type *type, Token::Loc loc,
		   bool misusedAsMember = false);

    public:

	static ExprPtr create(UStr ident, Token::Loc loc = Token::Loc{});
	static ExprPtr create(UStr ident, const Type *type,
			      Token::Loc loc = Token::Loc{});

	const UStr ident;
	const bool misusedAsMember;

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

#endif // IDENTIFIER_HPP
