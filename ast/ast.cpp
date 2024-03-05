#include <unordered_map>
#include <iostream>

#include "expr/expr.hpp"
#include "gen/function.hpp"
#include "gen/instruction.hpp"
#include "gen/label.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "symtab/symtab.hpp"
#include "type/enumtype.hpp"
#include "type/structtype.hpp"
#include "type/typealias.hpp"

#include "ast.hpp"

namespace abc {

static UStr
unusedFilter(UStr name)
{
    if (!name.empty() && *name.c_str() == '.') {
	return UStr{};
    }
    return name;
}

//------------------------------------------------------------------------------

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
	    error::out() << unusedFilter(fnParamName[i].val);
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
    error::out() << ";\n\n";
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
	error::out() << unusedFilter(fnParamName[i].val);
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
    error::out() << "\n";
    error::out(indent) << "{\n";
    if (body) {
	body->print(indent + 4);
    }
    error::out(indent) << "}\n\n";
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
	error::out() << "\n";
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
    error::out() << "\n";
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
 * AstGlobalVar
 */
AstGlobalVar::AstGlobalVar(AstListPtr &&decl)
    : decl{std::move(*decl)}
{
}

void
AstGlobalVar::print(int indent) const
{
    error::out(indent) << "global ";
    if (decl.size() > 1) {
	error::out() << "\n";
	for (std::size_t i = 0; const auto &item : decl.node) {
	    auto var = dynamic_cast<const AstVar *>(item.get());
	    var->print(indent + 4);
	    if (i + 1< decl.size()) {
		error::out() << ", ";
	    } else {
		error::out() << "; ";
	    }
	    ++i;
	}
    } else {
	auto var = dynamic_cast<const AstVar *>(decl.node[0].get());
	var->print(0);
	error::out() << "; ";
    }
    error::out() << "\n";
}

void
AstGlobalVar::codegen()
{
    for (const auto &item : decl.node) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	gen::globalVariableDefinition(var->varId.c_str(), var->varType, nullptr,
				      false);
    }
}

/*
 * AstLocalVar
 */
AstLocalVar::AstLocalVar(AstListPtr &&decl)
    : decl{std::move(*decl)}
{
}

void
AstLocalVar::print(int indent) const
{
    error::out(indent) << "local ";
    if (decl.size() > 1) {
	for (std::size_t i = 0; const auto &item : decl.node) {
	    error::out() << "\n";
	    auto var = dynamic_cast<const AstVar *>(item.get());
	    var->print(indent + 4);
	    if (i + 1< decl.size()) {
		error::out() << ", ";
	    } else {
		error::out() << "; ";
	    }
	    ++i;
	}
    } else {
	auto var = dynamic_cast<const AstVar *>(decl.node[0].get());
	var->print(0);
	error::out() << "; ";
    }
    error::out() << "\n";
}

void
AstLocalVar::codegen()
{
    for (const auto &item : decl.node) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	gen::localVariableDefinition(var->varId.c_str(), var->varType);
    }
}

/*
 * AstReturn
 */
AstReturn::AstReturn(lexer::Loc loc, ExprPtr &&expr)
    : loc{loc}, expr{std::move(expr)}
{}

void
AstReturn::print(int indent) const
{
    error::out(indent) << "return";
    if (expr) {
	error::out() << " " << expr;
    }
    error::out() << ";\n";
}

void
AstReturn::codegen()
{
    if (!expr) {
	return;
    }
    if (!gen::bbOpen()) {
	error::out() << loc << ": warning: return statement not reachabel\n";
	return;
    }
    gen::returnInstruction(expr->loadValue());
}

/*
 * AstExpr
 */
AstExpr::AstExpr(ExprPtr &&expr)
    : expr(std::move(expr))
{}

void
AstExpr::print(int indent) const
{
    if (expr) {
	error::out(indent) << expr << ";\n";
    } else {
	error::out(indent) << ";\n";
    }
}

void
AstExpr::codegen()
{
    if (!expr) {
	return;
    }
    if (!gen::bbOpen()) {
	error::out() << expr->loc
	    << ": warning: expression statement not reachabel\n";
	return;
    }
    expr->loadValue(); 
}

/*
 * AstIf
 */
AstIf::AstIf(lexer::Loc loc, ExprPtr &&cond, AstPtr &&thenBody)
    : loc{loc}, cond{std::move(cond)}, thenBody{std::move(thenBody)}
{}

AstIf::AstIf(lexer::Loc loc, ExprPtr &&cond, AstPtr &&thenBody,
	     AstPtr &&elseBody)
    : loc{loc}, cond{std::move(cond)}, thenBody{std::move(thenBody)}
    , elseBody{std::move(elseBody)}
{}

void
AstIf::print(int indent) const
{
    error::out(indent) << "if (" << cond << ") {\n";
    thenBody->print(indent + 4);
    if (elseBody) {
	error::out(indent) << "} else {\n";
	elseBody->print(indent + 4);
    }
    error::out(indent) << "}\n";
}

void
AstIf::codegen()
{
    if (!gen::bbOpen()) {
	error::out() << loc << ": warning: if statement not reachabel\n";
	return;
    }

    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    bool endLabelUsed = false;

    cond->condition(thenLabel, elseLabel);
    gen::defineLabel(thenLabel);
    thenBody->codegen();
    if (gen::bbOpen()) {
	endLabelUsed = true;
	gen::jumpInstruction(endLabel);
    }
    gen::defineLabel(elseLabel);
    if (elseBody) {
	elseBody->codegen();
    }
    if (gen::bbOpen()) {
	endLabelUsed = true;
	gen::jumpInstruction(endLabel); // connect with 'end'
					// (even if 'else' is empyt)
    }
    if (endLabelUsed) {
	gen::defineLabel(endLabel);
    }
}

void
AstIf::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	thenBody->apply(op);
	if (elseBody) {
	    elseBody->apply(op);
	}
    }
}

/*
 * AstTypeDecl
 */
AstTypeDecl::AstTypeDecl(lexer::Token name, const Type *type)
    : name{name}, type{type}
{
    auto aliasType = TypeAlias::create(name.val, type);
    auto add = Symtab::addType( name.loc, name.val, aliasType);
    if (!add.second) {
	error::out() << name.loc << ": error: '" << name.val
	    << "' already defined in this scope\n";
	error::fatal();
    }
}

void
AstTypeDecl::print(int indent) const
{
    error::out(indent) << "type " << name.val << ":" << type << "\n";
}

/*
 * AstEnumDecl
 */

AstEnumDecl::AstEnumDecl(lexer::Token enumTypeName, const Type *intType)
    : enumTypeName{enumTypeName}, intType{intType}
{
    if (!intType->isInteger() || intType->isEnum()) {
	error::out() << enumTypeName.loc
	    << ": error: enum type has to be an integer type\n";
	error::fatal();
    }
    enumType = EnumType::createIncomplete(enumTypeName.val, intType);
    Symtab::addType(enumTypeName.loc, enumTypeName.val, enumType);
}

void
AstEnumDecl::add(lexer::Token name)
{
    add(name, ExprPtr{});
}

void
AstEnumDecl::add(lexer::Token name, ExprPtr &&expr)
{
    if (expr) {
	assert(expr->isConst());
	assert(expr->type);
	assert(expr->type->isInteger());

	enumLastVal = expr->getSignedIntValue();
    }
    enumExpr.push_back(std::move(expr));

    auto ec = EnumConstant::create(name.val, enumLastVal++, enumType, name.loc);
    auto add = Symtab::addExpression(name.loc, name.val, ec.get());
    assert(add.second);
    enumConstant.push_back(std::move(ec));
}

void
AstEnumDecl::complete()
{
    std::vector<UStr> constName;
    std::vector<std::int64_t> constValue;

    for (std::size_t i = 0; i < enumConstant.size(); ++i) {
	auto p = dynamic_cast<const EnumConstant *>(enumConstant[i].get());
	assert(p);
	constName.push_back(p->name);
	constValue.push_back(p->value);
    }
    enumType->complete(std::move(constName), std::move(constValue));
}


void
AstEnumDecl::print(int indent) const
{
    error::out(indent) << "enum " << enumTypeName.val;
    if (intType) {
	error::out() << ": " << intType;
    }
    error::out() << "\n";
    error::out(indent) << "{\n";
    for (std::size_t i = 0; i < enumConstant.size(); ++i) {
	error::out(indent + 4) << enumConstant[i];
	if (enumExpr[i]) {
	    error::out() << " = " << enumExpr[i];
	}
	error::out() << ", // ";
	if (intType->isSignedInteger()) {
	    error::out() << enumConstant[i]->getSignedIntValue() << "\n";
	} else {
	    error::out() << enumConstant[i]->getUnsignedIntValue() << "\n";
	}
    }
    error::out(indent) << "}\n\n";
}

void
AstEnumDecl::codegen()
{
}

/*
 * AstStructDecl
 */
AstStructDecl::AstStructDecl(lexer::Token structTypeName)
    : structTypeName{structTypeName}
{
    structType = StructType::createIncomplete(structTypeName.val);
    Symtab::addType(structTypeName.loc, structTypeName.val, structType);
}

void
AstStructDecl::add(std::vector<lexer::Token> &&memberName,
		   const Type *memberType)
{
    assert(memberType);
    memberDecl.push_back({std::move(memberName), memberType});
}

void
AstStructDecl::add(std::vector<lexer::Token> &&memberName, AstPtr &&memberType)
{
    assert(memberType);
    memberDecl.push_back({std::move(memberName), std::move(memberType)});
}

void
AstStructDecl::complete()
{
    std::vector<UStr> memberName;
    std::vector<const Type *> memberType;

    for (const auto &decl: memberDecl) {
	const Type *type = nullptr;
	if (std::holds_alternative<const Type *>(decl.second)) {
	    type = std::get<const Type *>(decl.second);
	} else {
	    const auto ast = std::get<AstPtr>(decl.second).get();
	    const auto structDecl = dynamic_cast<AstStructDecl *>(ast);
	    assert(structDecl);
	    type = structDecl->getStructType();
	}

	for (std::size_t i = 0; i < decl.first.size(); ++i) {
	    memberName.push_back(decl.first[i].val);
	    memberType.push_back(type);
	}
    }

    structType->complete(std::move(memberName), std::move(memberType));
}

const Type *
AstStructDecl::getStructType() const
{
    return structType;
}

void
AstStructDecl::print(int indent) const
{
    if (!memberDecl.size()) {
	error::out(indent) << "struct " << structTypeName.val << ";\n";
    } else {
	error::out(indent) << "struct " << structTypeName.val << "\n";
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
	    if (std::holds_alternative<const Type *>(decl.second)) {
		error::out() << std::get<const Type *>(decl.second) << ";\n";
	    } else {
		error::out() << "\n";
		std::get<AstPtr>(decl.second)->print(indent + 8);
	    }
	}
	error::out(indent) << "};\n";
	if (indent==0) {
	    error::out() << "\n";
	}
    }
}

void
AstStructDecl::codegen()
{
}

} // namespace abc
