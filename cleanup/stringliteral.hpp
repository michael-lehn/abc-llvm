#ifndef STRINGLITERAL
#define STRINGLITERAL

#include <cstddef>

#include "expr.hpp"

class StringLiteral : public Expr
{
    protected:
	StringLiteral(UStr val, UStr valRaw, Token::Loc loc);

    public:
	static ExprPtr create(UStr val, UStr valRaw,
			      Token::Loc loc = Token::Loc{});

	UStr		    val, valRaw;
	UStr		    ident;

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
	virtual void printFlat(std::ostream &out, int prec) const override;
};

#endif // STRINGLITERAL
