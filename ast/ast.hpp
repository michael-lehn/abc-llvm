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
    public:
	AstFuncDef(lexer::Token fnName, const Type *fnType,
		   std::vector<lexer::Token> &&fnArgName);

	const lexer::Token fnName;
	const Type * const fnType;
	const std::vector<lexer::Token> fnArgName;
	UStr fnId;

	AstPtr body;

	void appendBody(AstPtr &&body);

	void print(int indent) const override;
	void codegen() override;
};


} // namespace abc

#endif // AST_HPP
