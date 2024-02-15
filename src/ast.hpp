#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <memory>
#include <vector>

#include "initializerlist.hpp"
#include "lexer.hpp"
#include "type.hpp"

class Ast
{
    public:
	Ast() = default;
	virtual ~Ast() = default;

	virtual void print(int indent = 0) const = 0;
	virtual void codegen() = 0;
	virtual void apply(std::function<bool(Ast *)> op);
};

using AstPtr = std::unique_ptr<Ast>;

//------------------------------------------------------------------------------

class AstList : public Ast
{
    public:
	std::vector<AstPtr> list;

	AstList() = default;

	std::size_t size() const;
	void append(AstPtr &&ast);
	void print(int indent = 0) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

using AstListPtr = std::unique_ptr<AstList>;

//------------------------------------------------------------------------------

class AstCompound : public Ast
{
    private:
	AstList list;

    public:
	AstCompound() = default;

	void append(AstPtr &&ast);

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstIf : public Ast
{
    public:
	AstIf(ExprPtr &&cond, AstPtr &&thenBody);
	AstIf(ExprPtr &&cond, AstPtr &&thenBody, AstPtr &&elseBody);

	const ExprPtr cond;
	const AstPtr thenBody;
	const AstPtr elseBody;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstSwitch : public Ast
{
    private:
	AstList body;
	std::vector<std::size_t> casePos;
	std::vector<ExprPtr> caseExpr;
	std::size_t defaultPos;
	bool hasDefault;

    public:
	AstSwitch(ExprPtr &&expr);

	const ExprPtr expr;

	void appendCase(ExprPtr &&expr);
	bool appendDefault();
	void append(AstPtr &&stmt);

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstWhile : public Ast
{
    public:
	AstWhile(ExprPtr &&cond, AstPtr &&body);

	const ExprPtr cond;
	const AstPtr body;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstFor : public Ast
{
    private:
	AstPtr body;

    public:
	AstFor(ExprPtr &&init, ExprPtr &&cond, ExprPtr &&update);
	~AstFor();

	const ExprPtr init, cond, update;

	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstReturn : public Ast
{
    public:
	AstReturn(ExprPtr &&expr);

	const Type *retType = nullptr;
	ExprPtr expr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstBreak : public Ast
{
    public:
	AstBreak(Token::Loc loc);

	const Token::Loc loc;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstContinue : public Ast
{
    public:
	AstContinue(Token::Loc loc);

	const Token::Loc loc;
	gen::Label label = nullptr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstExpr : public Ast
{
    public:
	AstExpr(ExprPtr &&expr);

	ExprPtr expr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstVar : public Ast
{
    public:
	AstVar(UStr ident, const Type *type, Token::Loc loc,
	       InitializerList &&init);

	const UStr ident;
	UStr genIdent; // used for code generation, set when defined
	const Type * const type;
	Token::Loc loc;
	const InitializerList init;
	bool externFlag = false;

	void print(int indent) const override;
	void codegen() override;
};

using AstVarPtr = std::unique_ptr<AstVar>;

//------------------------------------------------------------------------------

class AstGlobalVar : public Ast
{
    public:
	AstGlobalVar(AstListPtr &&decl);

	const AstList decl;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstLocalVar : public Ast
{
    public:
	AstLocalVar(AstListPtr &&decl);

	const AstList decl;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDecl : public Ast
{
    public:
	AstFuncDecl(Token fnIdent, const Type *type,
		    std::vector<Token> paramToken,
		    bool externFlag = false);

	const Token fnIdent;
	const Type * const type;
	const std::vector<Token> paramToken;
	const bool externFlag;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDef : public Ast
{
    public:
	AstFuncDef(Token fnIdent, const Type *type,
		   std::vector<Token> paramToken, bool externFlag = false);
	~AstFuncDef();

	const Token fnIdent;
	const Type * const type;
	const std::vector<Token> paramToken;
	std::vector<const char *> paramIdent;
	AstPtr body;
	const bool externFlag;

	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
};

#endif // AST_HPP
