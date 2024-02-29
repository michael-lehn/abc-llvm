#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <memory>
#include <vector>

#include "lexer/token.hpp"
#include "type/type.hpp"

namespace abc {

class Ast
{
    public:
	Ast() = default;
	virtual ~Ast() = default;

	virtual void print(int indent = 0) const = 0;
	virtual void codegen();
	virtual void apply(std::function<bool(Ast *)> op);
	virtual const Type *type() const;
};

using AstPtr = std::unique_ptr<Ast>;

//------------------------------------------------------------------------------

class AstList : public Ast
{
    public:
	std::vector<AstPtr> node;

	AstList() = default;

	std::size_t size() const;
	void append(AstPtr &&ast);
	void print(int indent = 0) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

using AstListPtr = std::unique_ptr<AstList>;

//------------------------------------------------------------------------------

class AstFuncDecl : public Ast
{
    public:
	AstFuncDecl(lexer::Token fnName, const Type *fnType,
		    std::vector<lexer::Token> &&fnArgName,
		    bool externalLinkage);

	const lexer::Token fnName;
	const Type * const fnType;
	const std::vector<lexer::Token> fnArgName;
	const bool externalLinkage;
	UStr fnId;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDef : public Ast
{
    private:
	std::vector<lexer::Token> fnArgName;
	std::vector<const char *> fnArgId;
	AstPtr body;

    public:
	AstFuncDef(lexer::Token fnName, const Type *fnType);

	const lexer::Token fnName;
	const Type * const fnType;
	UStr fnId;


	void appendArgName(std::vector<lexer::Token> &&fnArgName);
	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstVar : public Ast
{
    public:
	AstVar(lexer::Token varName, const Type *varType);

	const lexer::Token varName;
	const Type * const varType;
	UStr varId;

	void print(int indent) const override;
};

using AstVarPtr = std::unique_ptr<AstVar>;

//------------------------------------------------------------------------------

class AstExternVar : public Ast
{
    public:
	AstExternVar(AstListPtr &&declList);

	const AstListPtr declList;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstReturn : public Ast
{
    public:
	AstReturn(ExprPtr &&expr);

	ExprPtr expr;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

} // namespace abc

#endif // AST_HPP
