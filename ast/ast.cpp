#include <unordered_map>
#include <iostream>

#include "expr/expr.hpp"
#include "expr/compoundexpr.hpp"
#include "expr/implicitcast.hpp"
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

static std::function<bool(Ast *)>
createSetBreakLabel(gen::Label breakLabel)
{
    return [=](Ast *ast) -> bool
    {
	if (auto astBreak = dynamic_cast<AstBreak *>(ast)) {
	    if (!astBreak->label) {
		astBreak->label = breakLabel;
	    }
	}
	return true;
    };
}

static std::function<bool(Ast *)>
createSetContinueLabel(gen::Label continueLabel)
{
    return [=](Ast *ast) -> bool
    {
	if (auto astContinue = dynamic_cast<AstContinue *>(ast)) {
	    if (!astContinue->label) {
		astContinue->label = continueLabel;
	    }
	}
	return true;
    };
}

static std::function<bool(Ast *)>
createSetReturnType(const Type *retType)
{
    return [=](Ast *ast) -> bool
    {
	if (auto astReturn = dynamic_cast<AstReturn *>(ast)) {
	    astReturn->retType = retType;
	}
	return true;
    };
}

static std::function<bool(Ast *)>
createFindLabel(std::unordered_map<UStr, gen::Label> &label)
{
    return [&](Ast *ast) -> bool
    {
	if (auto astLabel = dynamic_cast<AstLabel *>(ast)) {
	    if (label.contains(astLabel->labelName)) {
		error::location(astLabel->loc);
		error::out() << error::setColor(error::BOLD) << astLabel->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "label already defined within function\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
	    } else {
		label[astLabel->labelName] = astLabel->label;
	    }
	}
	return true;
    };
}

static std::function<bool(Ast *)>
createSetGotoLabel(const std::unordered_map<UStr, gen::Label> &label)
{
    return [&](Ast *ast) -> bool
    {
	if (auto astLabel = dynamic_cast<AstGoto *>(ast)) {
	    if (!label.contains(astLabel->labelName)) {
		error::location(astLabel->loc);
		error::out() << error::setColor(error::BOLD) << astLabel->loc
		    << ": " << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << ": error: label not defined within function\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
	    } else {
		astLabel->label = label.at(astLabel->labelName);
	    }
	}
	return true;
    };
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
	error::out() << "\n";
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
    error::out() << ";";
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
    std::unordered_map<UStr, gen::Label> label;

    assert(!body);
    body = std::move(body_);
    body->apply(createSetReturnType(fnType->retType()));
    body->apply(createFindLabel(label));
    body->apply(createSetGotoLabel(label));
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
    error::out(indent) << "}";
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
    if (!gen::functionDefinitionEnd()) {
	error::location(fnName.loc);
	error::out() << error::setColor(error::BOLD) << fnName.loc
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "non-void function does not return a value in all control "
	    << "paths\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
}

/*
 * AstInitializerExpr
 */

AstInitializerExpr::AstInitializerExpr(const Type *destType, ExprPtr &&expr)
    : expr{ImplicitCast::create(std::move(expr), destType)}  
{
}

void
AstInitializerExpr::print(int indent) const
{
    if (expr) {
	error::out(indent) << expr;
    }
}

/*
 * AstVar
 */
AstVar::AstVar(lexer::Token varName, lexer::Loc varTypeLoc, const Type *varType,
	       bool define)
    : varName{varName}, varTypeLoc{varTypeLoc}, varType{varType}
{
    getId(define);
}

AstVar::AstVar(std::vector<lexer::Token> &&varName, lexer::Loc varTypeLoc,
	       const Type *varType, bool define)
    : varName{std::move(varName)}, varTypeLoc{varTypeLoc}, varType{varType}
{
    getId(define);
}

void
AstVar::getId(bool define)
{
    assert(varType);
    assert(!varName.empty());
    if (!varType->hasSize()) {
	error::location(varTypeLoc);
	error::out() << error::setColor(error::BOLD) << varTypeLoc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "type '" << varType << "' is incomplete\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
    for (std::size_t i = 0; i < varName.size(); ++i) {
	auto addDecl = define
	    ? Symtab::addDefinition(varName[i].loc, varName[i].val, varType)
	    : Symtab::addDeclaration(varName[i].loc, varName[i].val, varType);
	if (addDecl.first) {
	    varId.push_back(addDecl.first->id);
	} else if (define) {
	    std::cerr << "already defined\n";
	}
    }
}

void
AstVar::addInitializerExpr(AstInitializerExprPtr &&initializerExpr_)
{
    initializerExpr = std::move(initializerExpr_);
}

const Expr *
AstVar::getInitializerExpr() const
{
    if (!initializerExpr) {
	return nullptr;
    } else {
	return initializerExpr->expr.get();
    }
}

void
AstVar::print(int indent) const
{
    error::out(indent) << "";
    for (std::size_t i = 0; i < varName.size(); ++i) {
	error::out() << varName[i].val;
	if (i + 1 < varName.size()) {
	    error::out(indent) << ", ";
	}
    }
    error::out(indent) << ": " << varType;
    if (initializerExpr) {
	error::out() << " = ";
	initializerExpr->print(0);
    }
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
}

void
AstExternVar::codegen()
{
    for (const auto &decl : declList->node) {
	auto var = dynamic_cast<const AstVar *>(decl.get());
	assert(var);
	for (const auto &id: var->varId) {
	    gen::externalVariableDeclaration(id.c_str(), var->varType);
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
	auto initializer = var->getInitializerExpr();
	if (initializer && !initializer->isConst()) {
	    error::location(initializer->loc);
	    error::out() << error::setColor(error::BOLD) << initializer->loc
		<< ": " << error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "initializer is not a compile-time constant\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
	if (var->varId.size() == 1) {
	    auto initialValue = initializer
		? initializer->loadConstant()
		: nullptr;
	    gen::globalVariableDefinition(var->varId[0].c_str(), var->varType,
					  initialValue, false);
	} else {
	    auto compExpr = dynamic_cast<const CompoundExpr *>(initializer);
	    assert(!initializer || compExpr);
	    for (std::size_t i = 0; i < var->varId.size(); ++i) {
		auto initialValue = initializer
		    ? compExpr->loadConstant(i)
		    : nullptr;
		gen::globalVariableDefinition(var->varId[i].c_str(),
					      var->varType,
					      initialValue,
					      false);
	    }
	}
    }
}

/*
 * AstStaticVar
 */
AstStaticVar::AstStaticVar(AstListPtr &&decl)
    : decl{std::move(*decl)}
{
}

void
AstStaticVar::print(int indent) const
{
    error::out(indent) << "static ";
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
AstStaticVar::codegen()
{
    for (const auto &item : decl.node) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	auto initializer = var->getInitializerExpr();
	if (initializer && !initializer->isConst()) {
	    error::location(initializer->loc);
	    error::out() << error::setColor(error::BOLD) << initializer->loc
		<< ": " << error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "initializer is not a compile-time constant\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	}
	if (var->varId.size() == 1) {
	    auto initialValue = initializer
		? initializer->loadConstant()
		: nullptr;
	    gen::globalVariableDefinition(var->varId[0].c_str(), var->varType,
					  initialValue, false);
	} else {
	    auto compExpr = dynamic_cast<const CompoundExpr *>(initializer);
	    assert(compExpr);
	    for (std::size_t i = 0; i < var->varId.size(); ++i) {
		gen::globalVariableDefinition(var->varId[i].c_str(),
					      var->varType,
					      compExpr->loadConstant(i),
					      false);
	    }
	}
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
}

void
AstLocalVar::codegen()
{
    for (const auto &item : decl.node) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	auto initializer = var->getInitializerExpr();
	if (var->varId.size() == 1) {
	    gen::localVariableDefinition(var->varId[0].c_str(), var->varType);
	    if (initializer) {
		gen::store(initializer->loadValue(),
			   gen::loadAddress(var->varId[0].c_str()));
	    }
	} else {
	    auto compExpr = dynamic_cast<const CompoundExpr *>(initializer);
	    assert(!initializer || compExpr);
	    for (std::size_t i = 0; i < var->varId.size(); ++i) {
		gen::localVariableDefinition(var->varId[i].c_str(),
					     var->varType);
		if (initializer) {
		    gen::store(compExpr->loadValue(i),
			       gen::loadAddress(var->varId[i].c_str()));
		}
	    }
	}
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
    error::out() << ";";
}

void
AstReturn::codegen()
{
    assert(retType);
    if (!gen::bbOpen()) {
	error::location(loc);
	error::out() << loc << ": warning: return statement not reachabel\n";
	return;
    }
    if (retType->isVoid()) {
	if (expr) {
	    error::location(expr->loc);
	    error::out() << error::setColor(error::BOLD) << expr->loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< ": void function should not return a value\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return;
	}
	gen::returnInstruction(nullptr);
    } else {
	if (!expr) {
	    error::location(expr->loc);
	    error::out() << error::setColor(error::BOLD) << expr->loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< ": non-void function should return a value\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return;
	}
	expr = ImplicitCast::create(std::move(expr), retType);
	gen::returnInstruction(expr->loadValue());
    }
}

/*
 * AstGoto
 */
AstGoto::AstGoto(lexer::Loc loc, UStr labelName)
    : loc{loc}, labelName{labelName}
{}

void
AstGoto::print(int indent) const
{
    error::out(indent) << "goto " << labelName.c_str() << ";";
}

void
AstGoto::codegen()
{
    if (!label) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << " no label '" << labelName.c_str()
	    << "' within function\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    } else {
	gen::jumpInstruction(label);
    }
}

/*
 * AstLabel
 */
AstLabel::AstLabel(lexer::Loc loc, UStr labelName)
    : loc{loc}, labelName{labelName}, label{gen::getLabel(labelName.c_str())}
{}

void
AstLabel::print(int indent) const
{
    error::out(indent) << "label " << labelName.c_str() << ":";
}

void
AstLabel::codegen()
{
    gen::defineLabel(label);
}

/*
 * AstBreak
 */
AstBreak::AstBreak(lexer::Loc loc)
    : loc{loc}
{}

void
AstBreak::print(int indent) const
{
    error::out(indent) << "break;";
}

void
AstBreak::codegen()
{
    if (!label) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": break statement not within loop or switch\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return;
    }
    gen::jumpInstruction(label);
}

/*
 * AstContinue
 */
AstContinue::AstContinue(lexer::Loc loc)
    : loc{loc}
{}

void
AstContinue::print(int indent) const
{
    error::out(indent) << "continue;";
}

void
AstContinue::codegen()
{
    if (!label) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "continue statement not within loop\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return;
    }
    gen::jumpInstruction(label);
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
	error::out(indent) << expr << ";";
    } else {
	error::out(indent) << ";";
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
	if (auto astIf = dynamic_cast<const AstIf *>(elseBody.get())) {
	    error::out(indent) << "} else ";
	    astIf->printElseIfCase(indent);
	} else {
	    error::out(indent) << "} else {\n";
	    elseBody->print(indent + 4);
	    error::out(indent) << "}";
	}
    } else {
	error::out(indent) << "}";
    }
}

void
AstIf::printElseIfCase(int indent) const
{
    error::out() << "if (" << cond << ") {\n";
    thenBody->print(indent + 4);
    if (elseBody) {
	if (auto astIf = dynamic_cast<const AstIf *>(elseBody.get())) {
	    error::out(indent) << "} else ";
	    astIf->printElseIfCase(indent);
	} else {
	    error::out(indent) << "} else {\n";
	    elseBody->print(indent + 4);
	    error::out(indent) << "}";
	}
    } else {
	error::out(indent) << "}";
    }
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
 * AstSwitch
 */
AstSwitch::AstSwitch(ExprPtr &&expr)
    : hasDefault{false}, expr{std::move(expr)}
{}

void
AstSwitch::appendCase(ExprPtr &&caseExpr_)
{
    if (!caseExpr_ || !caseExpr_->isConst() || !caseExpr_->type->isInteger()) {
	error::location(caseExpr_->loc);
	error::out() << error::setColor(error::BOLD) << caseExpr_->loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": case expression has "
	    << "to be a constant integer expression\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
    caseExpr_ = ImplicitCast::create(std::move(caseExpr_), expr->type);
    casePos.push_back(body.size());
    caseExpr.push_back(std::move(caseExpr_));
}

bool
AstSwitch::appendDefault()
{
    if (hasDefault) {
	return false;
    }
    defaultPos = body.size();
    hasDefault = true;
    return true;
}

void
AstSwitch::append(AstPtr &&stmt)
{
    body.append(std::move(stmt));
}

void
AstSwitch::complete()
{
    auto bs = body.size();
    if ((casePos.size() && casePos.back() == bs)
	    || (hasDefault && defaultPos == bs)) {
	body.append(std::make_unique<AstExpr>(ExprPtr{}));
    }
}

void
AstSwitch::print(int indent) const
{
    error::out(indent) << "switch (" << expr << ") {\n";
    for (std::size_t i = 0, casePosIndex = 0; i < body.size(); ++i) {
	if ((!casePos.size() || (casePos.size() && i < casePos[0]))
		&& (!hasDefault || (hasDefault && i < defaultPos))) {
	    error::out(indent + 4) << "// never reached\n";
	}
	while (casePosIndex < casePos.size() && i == casePos[casePosIndex]) {
	    error::out(indent + 4) << "case " << caseExpr[casePosIndex++]
		<< ":\n";
	}
	if (hasDefault && i == defaultPos) {
	    error::out(indent + 4) << "default:\n";
	}
	body.node[i]->print(indent + 8);
	error::out() << "\n";
    }
    error::out(indent) << "}";
}

void
AstSwitch::codegen()
{
    auto defaultLabel = gen::getLabel("default");
    auto breakLabel = gen::getLabel("break");
    body.apply(createSetBreakLabel(breakLabel));

    std::vector<std::pair<gen::ConstantInt, gen::Label>> caseLabel;
    std::set<std::uint64_t> usedCaseVal;

    for (const auto &e: caseExpr) {
	caseLabel.push_back({ e->getConstantInt(), gen::getLabel("case") });
	auto val = e->getUnsignedIntValue();
	if (usedCaseVal.contains(val)) {
	    error::out() << e->loc << ": duplicate case value '" << e << "'\n";
	    error::fatal();
	}
	usedCaseVal.insert(val);
    }

    gen::jumpInstruction(expr->loadValue(), defaultLabel, caseLabel);

    for (std::size_t i = 0, casePosIndex = 0; i < body.size(); ++i) {
	while (casePosIndex < casePos.size() && i == casePos[casePosIndex]) {
	    gen::defineLabel(caseLabel[casePosIndex++].second);
	}
	if (hasDefault && i == defaultPos) {
	    gen::defineLabel(defaultLabel);
	}
	body.node[i]->codegen();
    }
    if (!hasDefault) {
	gen::defineLabel(defaultLabel);
    }
    gen::defineLabel(breakLabel);
}

void
AstSwitch::apply(std::function<bool(Ast *)> op)
{
    body.apply(op);
}

/*
 * AstWhile
 */
AstWhile::AstWhile(ExprPtr &&cond, AstPtr &&body)
    : cond{std::move(cond)}, body{std::move(body)}
{}

void
AstWhile::print(int indent) const
{
    error::out(indent) << "while (" << cond << ") {\n";
    body->print(indent + 4);
    error::out(indent) << "}";
}

void
AstWhile::codegen()
{
    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    body->apply(createSetBreakLabel(endLabel));
    body->apply(createSetContinueLabel(condLabel));

    gen::defineLabel(condLabel);
    cond->condition(loopLabel, endLabel);

    gen::defineLabel(loopLabel);
    body->codegen();
    gen::jumpInstruction(condLabel);

    gen::defineLabel(endLabel);
}

void
AstWhile::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	body->apply(op);
    }
}

/*
 * AstDoWhile
 */
AstDoWhile::AstDoWhile(ExprPtr &&cond, AstPtr &&body)
    : cond{std::move(cond)}, body{std::move(body)}
{}

void
AstDoWhile::print(int indent) const
{
    error::out(indent) << "do {\n";
    body->print(indent + 4);
    error::out(indent) << "} while (" << cond << ");";
}

void
AstDoWhile::codegen()
{
    auto loopLabel = gen::getLabel("loop");
    auto condLabel = gen::getLabel("cond");
    auto endLabel = gen::getLabel("end");

    body->apply(createSetBreakLabel(endLabel));
    body->apply(createSetContinueLabel(condLabel));

    gen::defineLabel(loopLabel);
    body->codegen();

    gen::defineLabel(condLabel);
    cond->condition(loopLabel, endLabel);

    gen::defineLabel(endLabel);
}

void
AstDoWhile::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	body->apply(op);
    }
}

/*
 * AstFor
 */
AstFor::AstFor(ExprPtr &&init, ExprPtr &&cond, ExprPtr &&update)
    : initExpr{std::move(init)}, cond{std::move(cond)}
    , update{std::move(update)}
{
}

AstFor::AstFor(AstPtr &&init, ExprPtr &&cond, ExprPtr &&update)
    : initAst{std::move(init)}, cond{std::move(cond)}
    , update{std::move(update)}
{
}

void
AstFor::appendBody(AstPtr &&body_)
{
    body = std::move(body_);
}

void
AstFor::print(int indent) const
{
    error::out(indent) << "for (";
    if (initAst) {
	initAst->print(0);
    } else if (initExpr) {
	error::out() << initExpr;
	error::out() << "; ";
    }
    if (cond) {
	error::out() << cond;
    }
    error::out() << "; ";
    if (update) {
	error::out() << update;
    }
    error::out() << ") {\n";
    body->print(indent + 4);
    error::out(indent) << "}";
}

void
AstFor::codegen()
{
    auto condLabel = gen::getLabel("cond");
    auto loopLabel = gen::getLabel("loop");
    auto endLabel = gen::getLabel("end");

    body->apply(createSetBreakLabel(endLabel));
    body->apply(createSetContinueLabel(condLabel));

    if (initAst) {
	initAst->codegen();
    } else if (initExpr) {
	initExpr->loadValue();
    }

    gen::defineLabel(condLabel);
    if (cond) {
	cond->condition(loopLabel, endLabel);
    }

    gen::defineLabel(loopLabel);
    body->codegen();
    if (update) {
	update->loadValue();
    }
    gen::jumpInstruction(condLabel);

    gen::defineLabel(endLabel);
}

void
AstFor::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	body->apply(op);
    }
}

/*
 * AstTypeDecl
 */
AstTypeDecl::AstTypeDecl(lexer::Token name, const Type *type)
    : name{name}, type{type}
{
    if (auto found = Symtab::type(name.val, Symtab::CurrentScope)) {
	// if <name> is already a type declaration it has to be an
	// identical alias for type
	if (found->type->getUnalias() != type) {
	    error::location(name.loc);
	    error::out() << error::setColor(error::BOLD) << name.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "redefinition of '"
		<< name.val << "\n"
		<< error::setColor(error::NORMAL);
	    error::out() << found->loc
		<< ": note: previous definition is here\n";
	    error::fatal();
	}
    } else {
	// otherwise a new type alias gets created and added. If <name>
	// is a different kind of symbol (variable name or enum constatnt)
	// the error is handled by Symtab::addType
	auto aliasType = TypeAlias::create(name.val, type);
	auto add = Symtab::addType(name.loc, name.val, aliasType);
	assert(add.second);
    }
}

void
AstTypeDecl::print(int indent) const
{
    error::out(indent) << "type " << name.val << ":" << type << ";";
}

/*
 * AstEnumDecl
 */

AstEnumDecl::AstEnumDecl(lexer::Token name, const Type *intType)
    : enumTypeName{name}, intType{intType}
{
    if (!intType->isInteger() || intType->isEnum()) {
	error::location(enumTypeName.loc);
	error::out() << error::setColor(error::BOLD) << enumTypeName.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "enum type has to be an integer type\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }

    if (auto found = Symtab::type(name.val, Symtab::CurrentScope)) {
	// if <name> is already a type declaration it has to be an incomplete
	// enum type
	if (!found->type->isEnum() || found->type->hasSize()) {
	    error::location(name.loc);
	    error::out() << error::setColor(error::BOLD) << name.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "redefinition of '"
		<< name.val << "\n"
		<< error::setColor(error::NORMAL);
	    error::out() << error::setColor(error::BOLD) << found->loc
		<< ": note: previous definition is here\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	} else {
	    // grrh, const_cast! But we have to complete the type ...
	    enumType = const_cast<Type *>(found->type);
	}
    } else {
	// otherwise a new incomplete struct tyoe gets created and added. If
	// <name> is a different kind of symbol (variable name or enum
	// constatnt) the error is handled by Symtab::addType
	enumType = EnumType::createIncomplete(name.val, intType);
	auto add = Symtab::addType(name.loc, name.val, enumType);
	assert(add.second);
    }
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
    if (!enumConstant.size()) {
	error::out(indent) << "enum " << enumTypeName.val;
	if (intType) {
	    error::out() << ": " << intType;
	}
	error::out() << ";\n";
    } else {
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
	error::out(indent) << "};";
    }
}

void
AstEnumDecl::codegen()
{
}

/*
 * AstStructDecl
 */
AstStructDecl::AstStructDecl(lexer::Token name)
    : structTypeName{name}
{
    if (auto found = Symtab::type(name.val, Symtab::CurrentScope)) {
	// if <name> is already a type declaration it has to be an incomplete
	// struct type
	if (!found->type->isStruct() || found->type->hasSize()) {
	    error::location(name.loc);
	    error::out() << error::setColor(error::BOLD) << name.loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "redefinition of '"
		<< name.val << "\n"
		<< error::setColor(error::NORMAL);
	    error::out() << error::setColor(error::BOLD) << found->loc
		<< ": note: previous definition is here\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	} else {
	    // grrh, const_cast! But we have to complete the type ...
	    structType = const_cast<Type *>(found->type);
	}
    } else {
	// otherwise a new incomplete struct tyoe gets created and added. If
	// <name> is a different kind of symbol (variable name or enum
	// constatnt) the error is handled by Symtab::addType
	structType = StructType::createIncomplete(name.val);
	auto add = Symtab::addType(name.loc, name.val, structType);
	assert(add.second);
    }
}

void
AstStructDecl::add(std::vector<lexer::Token> &&memberName,
		   std::vector<std::size_t> &&memberIndex, 
		   const Type *memberType)
{
    assert(memberType);
    memberDecl.push_back({std::move(memberName), memberType});
    for (auto index: memberIndex) {
	this->memberIndex.push_back(index);
    }
}

void
AstStructDecl::add(std::vector<lexer::Token> &&memberName,
		   std::vector<std::size_t> &&memberIndex,
		   AstPtr &&memberType)
{
    assert(memberType);
    memberDecl.push_back({std::move(memberName), std::move(memberType)});
    for (auto index: memberIndex) {
	this->memberIndex.push_back(index);
    }
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

    structType->complete(std::move(memberName),
			 std::vector<std::size_t>{memberIndex},
			 std::move(memberType));
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
	error::out(indent) << "struct " << structTypeName.val << ";";
    } else {
	error::out(indent) << "struct " << structTypeName.val << "\n";
	error::out(indent) << "{\n";

	std::size_t pos = 0;
	bool unionSection = false;
	for (const auto &decl: memberDecl) {
	    bool lastPos = pos + 1 == memberIndex.size();
	    if (!unionSection) {
		if (!lastPos && memberIndex[pos] == memberIndex[pos + 1]) {
		    unionSection = true;
		    indent += 4;
		    error::out(indent) << "union {\n";
		}
	    }
	    error::out(indent + 4) << "";
	    for (std::size_t i = 0; i < decl.first.size(); ++i, ++pos) {
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
		error::out() << "\n";
	    }
	    if (unionSection) {
		if (lastPos || (memberIndex[pos - 1] != memberIndex[pos])) {
		    unionSection = false;
		    error::out(indent) << "};\n";
		    indent -= 4;
		}
	    }
	}
	error::out(indent) << "};";
    }
}

void
AstStructDecl::codegen()
{
}

} // namespace abc
