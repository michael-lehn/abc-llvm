#include <unordered_map>
#include <iostream>

#include "expr/expr.hpp"
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
		error::out() << astLabel->loc
		    << ": error: label already defined within function"
		    << std::endl;
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
		error::out() << astLabel->loc
		    << ": error: label not defined within function"
		    << std::endl;
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
    assert(retType);
    if (!gen::bbOpen()) {
	error::out() << loc << ": warning: return statement not reachabel\n";
	return;
    }
    if (retType->isVoid()) {
	if (expr) {
	    error::out() << expr->loc
		<< ": error: void function should not return a value\n";
	    error::fatal();
	    return;
	}
	gen::returnInstruction(nullptr);
    } else {
	if (!expr) {
	    error::out() << expr->loc
		<< ": error: non-void function should return a value\n";
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
    error::out(indent) << "goto " << labelName.c_str() << ";" << std::endl;
}

void
AstGoto::codegen()
{
    if (!label) {
	error::out() << loc << ": error: no label '" << labelName.c_str()
	    << "' within function" << std::endl;
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
    error::out(indent) << "label " << labelName.c_str() << ":" << std::endl;
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
    error::out(indent) << "break;" << std::endl;
}

void
AstBreak::codegen()
{
    if (!label) {
	error::out() << loc
	    << ": error: break statement not within loop or switch"
	    << std::endl;
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
    error::out(indent) << "continue;" << std::endl;
}

void
AstContinue::codegen()
{
    if (!label) {
	error::out() << loc
	    << ": error: continue statement not within loop"
	    << std::endl;
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
 * AstWhile
 */
AstWhile::AstWhile(ExprPtr &&cond, AstPtr &&body)
    : cond{std::move(cond)}, body{std::move(body)}
{}

void
AstWhile::print(int indent) const
{
    error::out(indent) << "while (" << cond << ") {" << std::endl;
    body->print(indent + 4);
    error::out(indent) << "}\n";
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
    error::out(indent) << "do {" << std::endl;
    body->print(indent + 4);
    error::out(indent) << "} while (" << cond << ");" << std::endl;
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
    }
    error::out() << "; ";
    if (cond) {
	error::out() << cond;
    }
    error::out() << "; ";
    if (update) {
	error::out() << update;
    }
    error::out() << ") {" << std::endl;
    body->print(indent + 4);
    error::out(indent) << "}\n";
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
	    error::out() << name.loc << ": error: redefinition of '"
		<< name.val << "\n";
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
    error::out(indent) << "type " << name.val << ":" << type << "\n";
}

/*
 * AstEnumDecl
 */

AstEnumDecl::AstEnumDecl(lexer::Token name, const Type *intType)
    : enumTypeName{name}, intType{intType}
{
    if (!intType->isInteger() || intType->isEnum()) {
	error::out() << enumTypeName.loc
	    << ": error: enum type has to be an integer type\n";
	error::fatal();
    }

    if (auto found = Symtab::type(name.val, Symtab::CurrentScope)) {
	// if <name> is already a type declaration it has to be an incomplete
	// enum type
	if (!found->type->isEnum() || found->type->hasSize()) {
	    error::out() << name.loc << ": error: redefinition of '"
		<< name.val << "\n";
	    error::out() << found->loc
		<< ": note: previous definition is here\n";
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
	error::out(indent) << "}\n\n";
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
	    error::out() << name.loc << ": error: redefinition of '"
		<< name.val << "\n";
	    error::out() << found->loc
		<< ": note: previous definition is here\n";
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
