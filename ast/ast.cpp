#include <unordered_map>
#include <iostream>

#include "gen/function.hpp"
#include "lexer/error.hpp"
#include "symtab/symtab.hpp"

#include "ast.hpp"

namespace abc {

/*
 * Ast
 */
void
Ast::apply(std::function<bool(Ast *)> op)
{
    op(this);
}

void
Ast::codegen()
{
}

const Type *
Ast::type() const
{
    return nullptr;
}

/*
 * AstList
 */
std::size_t
AstList::size() const
{
    return node.size();
}

void
AstList::append(AstPtr &&ast)
{
    node.push_back(std::move(ast));
}

void
AstList::print(int indent) const
{
    for (const auto &n: node) {
	n->print(indent);
    }
}

void
AstList::codegen()
{
    for (const auto &n: node) {
	n->codegen();
    }
}

void
AstList::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	for (const auto &n: node) {
	    n->apply(op);
	}
    }
}

/*
 * AstFunctionDecl
 */
AstFuncDecl::AstFuncDecl(lexer::Token fnName, const Type *fnType,
			 std::vector<lexer::Token> &&fnArgName,
			 bool externalLinkage)
    : fnName{fnName}, fnType{fnType}, fnArgName{std::move(fnArgName)}
    , externalLinkage{externalLinkage}
{
    assert(this->fnArgName.size() == this->fnType->argType().size());

    auto addDecl = Symtab::addDeclaration(fnName.loc, fnName.val, fnType);
    if (addDecl.first) {
	fnId = addDecl.first->id;
    }
}

void
AstFuncDecl::print(int indent) const
{
    error::out(indent) << (externalLinkage ? "extern " : "");
    error::out() << "fn " << fnName.val << "(";
    for (std::size_t i = 0; i < fnType->argType().size(); ++i) {
	error::out() << fnArgName[i].val;
	error::out() << ": " << fnType->argType()[i];
	if (i + 1 < fnType->argType().size()) {
	    error::out() << ", ";
	}
    }
    if (fnType->hasVarg()) {
	error::out() << ", ...";
    }
    error::out() << ")";
    if (!fnType->retType()->isVoid()) {
	error::out() << ": " << fnType->retType();
    }
    error::out() << ";" << std::endl << std::endl;
}

void
AstFuncDecl::codegen()
{
    if (fnId.c_str()) {
	gen::functionDeclaration(fnId.c_str(), fnType, externalLinkage);
    }
}

/*
 * AstFuncDef
 */
AstFuncDef::AstFuncDef(lexer::Token fnName, const Type *fnType,
		       std::vector<lexer::Token> &&fnArgName)
    : fnName{fnName}, fnType{fnType}, fnArgName{std::move(fnArgName)}
{
    assert(this->fnArgName.size() == this->fnType->argType().size());

    auto addDecl = Symtab::addDeclaration(fnName.loc, fnName.val, fnType);
    if (addDecl.first) {
	fnId = addDecl.first->id;
    }
}

void
AstFuncDef::print(int indent) const
{
    error::out(indent) << "fn " << fnName.val << "(";
    for (std::size_t i = 0; i < fnType->argType().size(); ++i) {
	error::out() << fnArgName[i].val;
	error::out() << ": " << fnType->argType()[i];
	if (i + 1 < fnType->argType().size()) {
	    error::out() << ", ";
	}
    }
    if (fnType->hasVarg()) {
	error::out() << ", ...";
    }
    error::out() << ")";
    if (!fnType->retType()->isVoid()) {
	error::out() << ": " << fnType->retType();
    }
    error::out() << std::endl;
    error::out(indent) << "{" << std::endl;
    if (body) {
	body->print(indent + 4);
    }
    error::out(indent) << "}" << std::endl;
}

void
AstFuncDef::codegen()
{
    if (!fnId.c_str()) {
	return;
    }
    std::vector<const char *> arg;
    for (const auto &name : fnArgName) {
	arg.push_back(name.val.c_str());
    }
    gen::functionDefinitionBegin(fnId.c_str(), fnType, arg, false);
    if (body) {
	body->codegen();
    }
    gen::functionDefinitionEnd();
}

} // namespace abc
