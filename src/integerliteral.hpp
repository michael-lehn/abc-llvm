#ifndef INTEGERLITERAL
#define INTEGERLITERAL

#include "expr.hpp"

class IntegerLiteral : public Expr
{
    protected:
	IntegerLiteral(UStr val, std::uint8_t  radix, const Type *type,
		       Token::Loc loc);

    public:
	static ExprPtr create(UStr val, std::uint8_t  radix,
			      const Type *type = nullptr,
			      Token::Loc loc = Token::Loc{});

	UStr		    val;
	const std::uint8_t  radix;

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
};

#endif // INTEGERLITERAL
