#include <unordered_map>
#include <iostream>

#include "expr/expr.hpp"
#include "gen/function.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
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
			 std::vector<lexer::Token> &&fnParamName,
			 bool externalLinkage)
    : fnName{fnName}, fnType{fnType}, fnParamName{std::move(fnParamName)}
    , externalLinkage{externalLinkage}
{
    assert(this->fnParamName.size() == this->fnType->paramType().size()
	    || this->fnParamName.size() == 0);

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
    for (std::size_t i = 0; i < fnType->paramType().size(); ++i) {
	if (i < fnParamName.size()) {
	    error::out() << fnParamName[i].val;
	}
	error::out() << ": " << fnType->paramType()[i];
	if (i + 1 < fnType->paramType().size()) {
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
AstFuncDef::AstFuncDef(lexer::Token fnName, const Type *fnType)
    : fnName{fnName}, fnType{fnType}
{
    auto addDecl = Symtab::addDeclaration(fnName.loc, fnName.val, fnType);
    if (addDecl.first) {
	fnId = addDecl.first->id;
    }
}

void
AstFuncDef::appendParamName(std::vector<lexer::Token> &&fnParamName_)
{
    fnParamName = std::move(fnParamName_);
    assert(fnParamName.size() == fnType->paramType().size());
    for (size_t i = 0; i < fnParamName.size(); ++i) {
	auto addDecl = Symtab::addDeclaration(fnParamName[i].loc,
					      fnParamName[i].val,
					      fnType->paramType()[i]);
	assert(addDecl.first);
	assert(addDecl.second);
	fnParamId.push_back(addDecl.first->id.c_str());
    }
}

void
AstFuncDef::appendBody(AstPtr &&body_)
{
    assert(!body);
    body = std::move(body_);
}

void
AstFuncDef::print(int indent) const
{
    error::out(indent) << "fn " << fnName.val << "(";
    for (std::size_t i = 0; i < fnType->paramType().size(); ++i) {
	error::out() << fnParamName[i].val;
	error::out() << ": " << fnType->paramType()[i];
	if (i + 1 < fnType->paramType().size()) {
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
    gen::functionDefinitionBegin(fnId.c_str(), fnType, fnParamId, false);
    if (body) {
	body->codegen();
    }
    gen::functionDefinitionEnd();
}

/*
 * AstVar
 */
AstVar::AstVar(lexer::Token varName, const Type *varType)
    : varName{varName}, varType{varType}
{
    auto addDecl = Symtab::addDeclaration(varName.loc, varName.val, varType);
    if (addDecl.first) {
	varId = addDecl.first->id;
    }
}

void
AstVar::print(int indent) const
{
    error::out(indent) << varName.val << ": " << varType;
}

/*
 * AstExternVar
 */
AstExternVar::AstExternVar(AstListPtr &&declList)
    : declList{std::move(declList)}
{
}

void
AstExternVar::print(int indent) const
{
    error::out(indent) << "extern ";
    if (declList->size() > 1) {
	error::out() << std::endl;
	for (std::size_t i = 0; const auto &decl : declList->node) {
	    auto var = dynamic_cast<const AstVar *>(decl.get());
	    assert(var);
	    var->print(indent + 4);
	    if (i + 1< declList->size()) {
		error::out() << ", ";
	    } else {
		error::out() << "; ";
	    }
	    ++i;
	}
    } else {
	auto var = dynamic_cast<const AstVar *>(declList->node[0].get());
	var->print(0);
	error::out() << "; ";
    }
    error::out() << std::endl;
}

void
AstExternVar::codegen()
{
    for (const auto &decl : declList->node) {
	auto var = dynamic_cast<const AstVar *>(decl.get());
	assert(var);
	if (var->varId.c_str()) {
	    gen::externalVariableiDeclaration(var->varId.c_str(),
					      var->varType);
	}
    }
}

/*
 * AstReturn
 */
AstReturn::AstReturn(ExprPtr &&expr)
    : expr{std::move(expr)}
{}

void
AstReturn::print(int indent) const
{
    error::out(indent) << "return";
    if (expr) {
	error::out() << " " << expr;
    }
    error::out() << ";" << std::endl;
}

void
AstReturn::codegen()
{
    if (expr) {
	gen::returnInstruction(expr->loadValue());
    }
}

} // namespace abc
