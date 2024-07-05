#include <cassert>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "typenode.hpp"

#include "lexer/error.hpp"
#include "symtab/symtab.hpp"
#include "type/arraytype.hpp"
#include "type/functiontype.hpp"
#include "type/pointertype.hpp"
#include "type/structtype.hpp"
#include "type/voidtype.hpp"

namespace abc {

static const Type *
getTypeFromToken(const lexer::Token &identifier)
{
    if (identifier.kind != lexer::TokenKind::IDENTIFIER) {
	return nullptr;
    }
    if (auto entry = Symtab::type(identifier.val, Symtab::AnyScope)) {
	return entry->type;
    }
    return nullptr;
}

static const Type *
getConstType(const Type *type)
{
    return type ? type->getConst() : nullptr;
}

static const Type *
getPointerType(const TypeNodePtr &refTypeNode)
{
    const Type *type = refTypeNode->type();
    return type ? PointerType::create(type) : nullptr;
}

static Type *
getStructType(const lexer::Token &structName)
{
    Type *type = nullptr;
    if (auto found = Symtab::type(structName.val, Symtab::CurrentScope)) {
	// if <structName> is already a type declaration it has to be an
	// incomplete struct type
	if (!found->type->isStruct() || found->type->hasSize()) {
	    std::stringstream ss;
	    ss << "incompatible redefinition of '" << structName.val << "\n";
	    error::fatal(structName.loc, ss.str().c_str());
	} else {
	    // grrh, const_cast! But we have to complete the type ...
	    type = const_cast<Type *>(found->type);
	}
    } else {
	// otherwise a new incomplete struct type gets created and added. If
	// <structName> is a different kind of symbol (variable name or enum
	// constatnt) the error is handled by Symtab::addType
	type = StructType::createIncomplete(structName.val);
	auto add = Symtab::addType(structName.loc, structName.val, type);
	assert(add.second);
    }
    return type;
}

static const Type *
getArrayType(const std::vector<ExprPtr> &dim, const TypeNodePtr &refTypeNode)
{
    const Type *type = refTypeNode->type();
    for (std::size_t i = 0; i < dim.size(); ++i) {
	if (!type) {
	    return nullptr;
	}
	auto dimVal = dim[i]
	    ? dim[i]->getUnsignedIntValue()
	    : 0;
	type = ArrayType::create(type, dimVal);
    }
    return type;
}

static const Type *
getFunctionType(std::vector<TypeNodePtr> &paramTypeNode, bool hasVargs,
		TypeNodePtr &retTypeNode)
{
    std::vector<const Type *> paramType;
    for (const auto &p: paramTypeNode) {
	paramType.push_back(p->type());
	if (!paramType.back()) {
	    return nullptr;
	}
    }
    auto retType = retTypeNode
	? retTypeNode->type()
	: VoidType::create();
    if (!retType) {
	return nullptr;
    }
    return FunctionType::create(retType, std::move(paramType), hasVargs);
}

//------------------------------------------------------------------------------

/*
 * TypeNode
 */
TypeNode::TypeNode(const Type *type)
    : type_{type}
{
}

const Type *
TypeNode::type() const
{
    return type_;
}

/*
 * IdentifierTypeNode
 */

IdentifierTypeNode::IdentifierTypeNode(lexer::Token identifier)
    : TypeNode(getTypeFromToken(identifier)), identifier{identifier}
{
    if (!type()) {
	error::fatal(identifier.loc, "not a type name");
    }
}

void
IdentifierTypeNode::print(int indent) const
{
    error::out(indent) << identifier.val;
}

/*
 * AstTypeReadonly
 */

ReadonlyTypeNode::ReadonlyTypeNode(TypeNodePtr &&refTypeNode)
    : TypeNode{getConstType(refTypeNode->type())}
    , refTypeNode{std::move(refTypeNode)}
{
}

void
ReadonlyTypeNode::print(int indent) const
{
    error::out(indent) << "readonly ";
    refTypeNode->print(0);
}

/*
 * PointerTypeNode
 */
PointerTypeNode::PointerTypeNode(TypeNodePtr &&refTypeNode)
    : TypeNode(getPointerType(refTypeNode))
    , refTypeNode{std::move(refTypeNode)}
{
}

void
PointerTypeNode::print(int indent) const
{
    error::out(indent) << "-> ";
    refTypeNode->print(0);
}

/*
 * ArrayTypeNode
 */
ArrayTypeNode::ArrayTypeNode(std::vector<ExprPtr> &&dim,
			     TypeNodePtr &&refTypeNode)
    : TypeNode(getArrayType(dim, refTypeNode)), dim{std::move(dim)}
    , refTypeNode{std::move(refTypeNode)}
{
}

void
ArrayTypeNode::print(int indent) const
{
    error::out(indent) << "array";
    for (std::size_t i = 0; i < dim.size(); ++i) {
	error::out(0) << "[";
	if (dim[i]) {
	    error::out(0) << dim[i];
	}
	error::out(0) << "]";
    }
    error::out(0) << " of ";
    refTypeNode->print(0);
}

/*
 * FunctionTypeNode
 */

FunctionTypeNode::FunctionTypeNode(std::optional<lexer::Token> fnName,
			           FnParamName &&fnParamName,
			           std::vector<TypeNodePtr> &&fnParamTypeNode,
			           bool hasVargs,
			           TypeNodePtr &&retTypeNode)
    : TypeNode{getFunctionType(fnParamTypeNode, hasVargs, retTypeNode)}
    , fnName{fnName}, fnParamName{std::move(fnParamName)}
    , fnParamTypeNode{std::move(fnParamTypeNode)}, hasVargs{hasVargs}
    , retTypeNode{std::move(retTypeNode)}
{
}

void
FunctionTypeNode::print(int indent) const
{
    error::out(indent) << "fn ";
    if (fnName) {
	error::out(0) << fnName.value().val;
    }
    error::out(0) << "(";
    for (std::size_t i = 0; i < fnParamTypeNode.size(); ++i) {
	if (fnParamName[i]) {
	    error::out(0) << fnParamName[i].value().val << ": ";
	}
	fnParamTypeNode[i]->print(0);
	if (hasVargs || i + 1 < fnParamTypeNode.size()) {
	    error::out(0) << ", ";
	}
    }
    if (hasVargs) {
	error::out(0) << "...";
    }
    error::out(0) << ")";
    if (retTypeNode && retTypeNode->type() && !retTypeNode->type()->isVoid()) {
	error::out(0) << ": ";
	retTypeNode->print(0);
    }
}

/*
 * StructTypeNode
 */

StructTypeNode::StructTypeNode(lexer::Token structName)
    : TypeNode{getStructType(structName)}
{
}

void
StructTypeNode::add(std::vector<lexer::Token> &&memberName,
		    TypeNodePtr typeNode)
{
    memberDecl.push_back({std::move(memberName), std::move(typeNode)});
}

void
StructTypeNode::complete()
{
    std::unordered_map<UStr, lexer::Loc> memberMap;
    std::vector<UStr> memberName;
    std::vector<std::size_t> memberIndex;
    std::vector<const Type *> memberType;

    for (const auto &decl: memberDecl) {
	const Type *type = decl.second->type();

	for (std::size_t i = 0; i < decl.first.size(); ++i) {
	    if (memberMap.contains(decl.first[i].val)) {
		std::stringstream ss;
		ss << "member '" << decl.first[i].val << "' already defined.";
		error::fatal(decl.first[i].loc, ss.str().c_str());
		error::fatal(memberMap[decl.first[i].val],
			     "previous definition here.");
	    }
	    memberMap[decl.first[i].val] = decl.first[i].loc;
	    memberName.push_back(decl.first[i].val);
	    memberType.push_back(type);
	    memberIndex.push_back(i);
	}
    }

    auto structType = dynamic_cast<StructType *>(const_cast<Type *>(type()));
    assert(structType);

    structType->complete(std::move(memberName),
			 std::move(memberIndex),
			 std::move(memberType));
}

void
StructTypeNode::print(int indent) const
{
    if (!memberDecl.size()) {
	error::out(indent) << "struct " << structName.val << ";";
    } else {
	error::out(indent) << "struct " << structName.val << "\n";
	error::out(indent) << "{\n";

	for (const auto &decl: memberDecl) {
	    error::out(indent + 4) << "";
	    for (std::size_t i = 0; i < decl.first.size(); ++i) {
		error::out() << decl.first[i].val;
		if (i + 1 < decl.first.size()) {
		    error::out() << ", ";
		}
	    }
	    error::out() << ": ";
	    decl.second->print(0);
	    error::out() << ";\n";
	}
	error::out(indent) << "};";
    }
}

} // namespace abc
