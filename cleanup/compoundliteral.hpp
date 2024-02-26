#ifndef COMPOUNDLITERAL_HPP
#define COMPOUNDLITERAL_HPP


#include "ast.hpp"
#include "expr.hpp"

class CompoundLiteral : public Expr
{
    protected:
	CompoundLiteral(AstInitializerListPtr &&ast, Token::Loc loc);

	bool createTmp() const;

    public:

	static ExprPtr create(AstInitializerListPtr &&ast, Token::Loc loc);

	UStr genIdent;
	const AstInitializerListPtr ast;
	const InitializerList initializerList;

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



#endif // COMPOUNDLITERAL_HPP
