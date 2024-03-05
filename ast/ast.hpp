#ifndef AST_HPP
#define AST_HPP

#include <functional>
#include <memory>
#include <vector>

#include "expr/expr.hpp"
#include "expr/enumconstant.hpp"
#include "lexer/loc.hpp"
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
		    std::vector<lexer::Token> &&fnParamName,
		    bool externalLinkage);

	const lexer::Token fnName;
	const Type * const fnType;
	const std::vector<lexer::Token> fnParamName;
	const bool externalLinkage;
	UStr fnId;

	void print(int indent) const override;
	void codegen() override;
};

//------------------------------------------------------------------------------

class AstFuncDef : public Ast
{
    private:
	std::vector<lexer::Token> fnParamName;
	std::vector<const char *> fnParamId;
	AstPtr body;

    public:
	AstFuncDef(lexer::Token fnName, const Type *fnType);

	const lexer::Token fnName;
	const Type * const fnType;
	UStr fnId;


	void appendParamName(std::vector<lexer::Token> &&fnParamName);
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

class AstReturn : public Ast
{
    public:
	AstReturn(lexer::Loc loc, ExprPtr &&expr);

	lexer::Loc loc;
	ExprPtr expr;

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

class AstIf : public Ast
{
    public:
	AstIf(lexer::Loc loc, ExprPtr &&cond, AstPtr &&thenBody);
	AstIf(lexer::Loc loc, ExprPtr &&cond, AstPtr &&thenBody,
	      AstPtr &&elseBody);

	lexer::Loc loc;
	const ExprPtr cond;
	const AstPtr thenBody;
	const AstPtr elseBody;

	void print(int indent) const override;
	void codegen() override;
	void apply(std::function<bool(Ast *)> op) override;
};

//------------------------------------------------------------------------------

class AstTypeDecl : public Ast
{
    public:
	AstTypeDecl(lexer::Token name, const Type *type);

	const lexer::Token name;
	const Type * const type;

	void print(int indent) const override;
};

//------------------------------------------------------------------------------

class AstEnumDecl : public Ast
{
    private:
	lexer::Token enumTypeName;
	const Type *intType;

	Type *enumType;
	std::int64_t enumLastVal = 0;
	std::vector<ExprPtr> enumExpr;	    // expr found by parser after '='
	std::vector<ExprPtr> enumConstant;  // gets referenced in symtab

    public:
	AstEnumDecl(lexer::Token enumTypeName, const Type *intType);

	void add(lexer::Token name);
	void add(lexer::Token name, ExprPtr &&expr);
	void complete();

	void print(int indent) const override;
	void codegen() override;
};

using AstEnumDeclPtr = std::unique_ptr<AstEnumDecl>;

//------------------------------------------------------------------------------

class AstStructDecl : public Ast
{
    private:
	lexer::Token structTypeName;

	using AstOrType = std::variant<AstPtr, const Type *>;
	using MemberDecl = std::pair<std::vector<lexer::Token>, AstOrType>;
	std::vector<MemberDecl> memberDecl;

    public:
	AstStructDecl(lexer::Token structTypeName);

	void add(std::vector<lexer::Token> &&memberName,
		 const Type *memberType);
	void add(std::vector<lexer::Token> &&memberName, AstPtr &&memberType);
	void complete();

	void print(int indent) const override;
	void codegen() override;
};

using AstStructDeclPtr = std::unique_ptr<AstStructDecl>;

} // namespace abc

#endif // AST_HPP
