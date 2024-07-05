#ifndef TYPENODE_HPP
#define TYPENODE_HPP

#include <memory>

#include "expr/expr.hpp"
#include "lexer/token.hpp"
#include "type/type.hpp"

namespace abc {

class TypeNode
{
    protected:
	const Type *type_;
	TypeNode(const Type *type);

    public:
	virtual ~TypeNode() = default;

	const Type *type() const;
	virtual void print(int indent = 0) const = 0;
};

using TypeNodePtr = std::unique_ptr<TypeNode>;

//------------------------------------------------------------------------------

class IdentifierTypeNode: public TypeNode
{
    private:
	lexer::Token identifier;

    public:
	IdentifierTypeNode(lexer::Token identifier);

	void print(int indent) const override;
};

//------------------------------------------------------------------------------

class ReadonlyTypeNode: public TypeNode
{
    private:
	TypeNodePtr refTypeNode;

    public:
	ReadonlyTypeNode(TypeNodePtr &&refTypeNode);

	void print(int indent) const override;
};

//------------------------------------------------------------------------------

class PointerTypeNode: public TypeNode
{
    private:
	TypeNodePtr refTypeNode;

    public:
	PointerTypeNode(TypeNodePtr &&refTypeNode);

	void print(int indent) const override;
};

//------------------------------------------------------------------------------

class ArrayTypeNode: public TypeNode
{
    private:
	std::vector<ExprPtr> dim;
	TypeNodePtr refTypeNode;

    public:

	ArrayTypeNode(std::vector<ExprPtr> &&dim, TypeNodePtr &&refTypeNode);

	void print(int indent) const override;

};

//------------------------------------------------------------------------------

class FunctionTypeNode: public TypeNode
{
    public:
	using FnParamName = std::vector<std::optional<lexer::Token>>;

    private:
	std::optional<lexer::Token> fnName;
	FnParamName fnParamName;
	std::vector<TypeNodePtr> fnParamTypeNode;
	bool hasVargs;
	TypeNodePtr retTypeNode;

    public:
	FunctionTypeNode(std::optional<lexer::Token> fnName,
		         FnParamName &&fnParamName,
		         std::vector<TypeNodePtr> &&fnParamTypeNode,
		         bool hasVargs,
		         TypeNodePtr &&retTypeNode);

	void print(int indent) const override;
};

//------------------------------------------------------------------------------

class StructTypeNode: public TypeNode
{
    private:
	lexer::Token structName;
	using MemberDecl = std::pair<std::vector<lexer::Token>, TypeNodePtr>;
	std::vector<MemberDecl> memberDecl;

    public:
	StructTypeNode(lexer::Token structName);

	void add(std::vector<lexer::Token> &&memberName, TypeNodePtr typeNode);
	void complete();

	void print(int indent) const override;
};

} // namespace abc

#endif // TYPENODE_HPP
