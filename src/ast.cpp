#include <iostream>

#include "ast.hpp"
#include "castexpr.hpp"
#include "gen.hpp"
#include "error.hpp"
#include "symtab.hpp"

/*
 * Ast
 */
void
Ast::apply(std::function<bool(Ast *)> op)
{
    op(this);
}

/*
 * AstTopLevel
 */
std::size_t
AstList::size() const
{
    return list.size();
}

void
AstList::append(AstPtr &&ast)
{
    list.push_back(std::move(ast));
}

void
AstList::print(int indent) const
{
    for (const auto &node: list) {
	node->print(indent);
    }
}

void
AstList::codegen()
{
    for (const auto &node: list) {
	node->codegen();
    }
}

void
AstList::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	for (const auto &node: list) {
	    node->apply(op);
	}
    }
}

/*
 * AstCompound
 */
void
AstCompound::append(AstPtr &&ast)
{
    list.append(std::move(ast));
}

void
AstCompound::print(int indent) const
{
    error::out(indent) << "{" << std::endl;
    list.print(indent + 4);
    error::out(indent) << "}" << std::endl;
}

void
AstCompound::codegen()
{
    list.codegen();
}

void
AstCompound::apply(std::function<bool(Ast *)> op)
{
    if (op(this)) {
	list.apply(op);
    }
}

/*
 * AstIf
 */
AstIf::AstIf(ExprPtr &&cond, AstPtr &&thenBody)
    : cond{std::move(cond)}, thenBody{std::move(thenBody)}
{}

AstIf::AstIf(ExprPtr &&cond, AstPtr &&thenBody, AstPtr &&elseBody)
    : cond{std::move(cond)}, thenBody{std::move(thenBody)}
    , elseBody{std::move(elseBody)}
{}

void
AstIf::print(int indent) const
{
    error::out(indent) << "if (" << cond << ")" << std::endl;
    thenBody->print(indent);
    if (elseBody) {
	error::out(indent) << "else" << std::endl;
	elseBody->print(indent);
    }
}

void
AstIf::codegen()
{
    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    cond->condJmp(thenLabel, elseLabel);
    gen::labelDef(thenLabel);
    thenBody->codegen();
    gen::jmp(endLabel);
    gen::labelDef(elseLabel);
    if (elseBody) {
	elseBody->codegen();
    }
    gen::jmp(endLabel); // connect with 'end' (even if 'else' is empyt)
    gen::labelDef(endLabel);
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
AstSwitch::appendCase(ExprPtr &&expr)
{
    casePos.push_back(body.size());
    caseExpr.push_back(std::move(expr));
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
AstSwitch::print(int indent) const
{
    error::out(indent) << "switch (" << expr << ") {" << std::endl;
    for (std::size_t i = 0, casePosIndex = 0; i < body.size(); ++i) {
	if (!casePos.size() || i < casePos[0]) {
	    error::out(indent + 4) << "// never reached" << std::endl;
	}
	if (casePosIndex < casePos.size() && i == casePos[casePosIndex]) {
	    error::out(indent + 4) << "case " << caseExpr[casePosIndex++]
		<< ":" << std::endl;
	}
	if (hasDefault && i == defaultPos) {
	    error::out(indent + 4) << "default:" << std::endl;
	}
	body.list[i]->print(indent + 8);
    }
    error::out(indent) << "}" << std::endl;
}

void
AstSwitch::codegen()
{
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
    error::out(indent) << "while (" << cond << ")" << std::endl;
    body->print(indent);
}

void
AstWhile::codegen()
{
}

/*
 * AstFor
 */
AstFor::AstFor(ExprPtr &&init, ExprPtr &&cond, ExprPtr &&update)
    : init{std::move(init)}, cond{std::move(cond)}, update{std::move(update)}
{
    Symtab::openScope();
}

AstFor::~AstFor()
{
    if (!body) {
	Symtab::closeScope();
    }
}

void
AstFor::appendBody(AstPtr &&body)
{
    this->body = std::move(body);
    Symtab::closeScope();
}

void
AstFor::print(int indent) const
{
    error::out(indent) << "for (";
    if (init) {
	error::out() << init;
    }
    error::out() << "; ";
    if (cond) {
	error::out() << cond;
    }
    error::out() << "; ";
    if (update) {
	error::out() << update;
    }
    error::out() << ")" << std::endl;
    body->print(indent);
}

void
AstFor::codegen()
{
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
    if (retType && !retType->isVoid()) {
	expr = CastExpr::create(std::move(expr), retType);
	gen::ret(expr->loadValue());
    } else {
	if (expr) {
	    error::out() << expr->loc
		<< ": error: function has return type 'void'" << std::endl;
	    error::fatal();
	}
	gen::ret();
    }
}

/*
 * AstBreak
 */
AstBreak::AstBreak()
{}

void
AstBreak::print(int indent) const
{
    error::out(indent) << "break;" << std::endl;
}

void
AstBreak::codegen()
{
}

/*
 * AstContinue
 */
AstContinue::AstContinue()
{}

void
AstContinue::print(int indent) const
{
    error::out(indent) << "continue;" << std::endl;
}

void
AstContinue::codegen()
{
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
    error::out(indent) << expr << ";" << std::endl;
}

void
AstExpr::codegen()
{
    expr->loadValue(); 
}

/*
 * AstVar
 */
AstVar::AstVar(UStr ident, const Type *type, Token::Loc loc,
			   InitializerList &&init)
    : ident{ident}, type{type}, loc{loc}, init{std::move(init)}
{
}

void
AstVar::print(int indent) const
{
    error::out(indent) << ident.c_str() << ":" << type;
    error::out() << " = [initializer]";
}

void
AstVar::codegen()
{
}

/*
 * AstGlobalVar
 */
AstGlobalVar::AstGlobalVar(AstListPtr &&decl)
    : decl{std::move(*decl)}
{
    for (auto &node : this->decl.list) {
	auto var = dynamic_cast<AstVar *>(node.get());
	assert(var);
	auto sym = Symtab::addDecl(var->loc, var->ident, var->type);
	sym->setDefinitionFlag();
	var->genIdent = sym->getInternalIdent();
	var->externFlag = sym->hasExternFlag();
    }
}

void
AstGlobalVar::print(int indent) const
{
    error::out(indent) << "global ";
    if (decl.size() > 1) {
	error::out() << std::endl;
	for (std::size_t i = 0; const auto &item : decl.list) {
	    auto var = dynamic_cast<const AstVar *>(item.get());
	    var->print(indent + 4);
	    if (i + 1< decl.size()) {
		error::out() << ", ";
	    } else {
		error::out() << "; ";
	    }
	    error::out() << "//" << var->genIdent.c_str() << std::endl;
	    ++i;
	}
    } else {
	auto var = dynamic_cast<const AstVar *>(decl.list[0].get());
	var->print(indent + 4);
	error::out() << "//" << var->genIdent.c_str() << ";" << std::endl;
    }
}

void
AstGlobalVar::codegen()
{
    for (const auto &item : decl.list) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	gen::defGlobal(var->genIdent.c_str(), var->type, true);
    }
}

/*
 * AstLocalVar
 */
AstLocalVar::AstLocalVar(AstListPtr &&decl)
    : decl{std::move(*decl)}
{
    for (auto &node : this->decl.list) {
	auto var = dynamic_cast<AstVar *>(node.get());
	assert(var);
	auto sym = Symtab::addDecl(var->loc, var->ident, var->type);
	sym->setDefinitionFlag();
	var->genIdent = sym->getInternalIdent();
    }
}

void
AstLocalVar::print(int indent) const
{
    error::out(indent) << "local ";
    if (decl.size() > 1) {
	error::out() << std::endl;
	for (std::size_t i = 0; const auto &item : decl.list) {
	    auto var = dynamic_cast<const AstVar *>(item.get());
	    var->print(indent + 4);
	    if (i + 1< decl.size()) {
		error::out() << ", ";
	    } else {
		error::out() << "; ";
	    }
	    error::out() << "//" << var->genIdent.c_str() << std::endl;
	    ++i;
	}
    } else {
	auto var = dynamic_cast<const AstVar *>(decl.list[0].get());
	var->print(indent + 4);
	error::out() << "//" << var->genIdent.c_str() << ";" << std::endl;
    }
}

void
AstLocalVar::codegen()
{
    for (const auto &item : decl.list) {
	auto var = dynamic_cast<const AstVar *>(item.get());
	assert(var);
	std::cerr << "define local variable " << var->genIdent.c_str()
	    << std::endl;
	gen::defLocal(var->genIdent.c_str(), var->type);
    }
}

/*
 * AstFunctionDecl
 */
AstFuncDecl::AstFuncDecl(Token fnIdent, const Type *type,
			 std::vector<Token> paramToken,
			 bool externFlag)
    : fnIdent{fnIdent}, type{type}, paramToken{paramToken}
    , externFlag{externFlag}
{
    auto sym = Symtab::addDecl(fnIdent.loc, fnIdent.val, type);
    if (externFlag) {
	sym->setExternFlag();
    }
}

void
AstFuncDecl::print(int indent) const
{
    error::out(indent)
	<< (externFlag ? "extern " : "")
	<< "fn " << fnIdent.val.c_str() << "(";
    for (std::size_t i = 0; i < type->getArgType().size(); ++i) {
	if (paramToken[i].val.c_str()) {
	    error::out() << paramToken[i].val.c_str();
	}
	error::out() << ": " << type->getArgType()[i];
	if (i + 1 < type->getArgType().size()) {
	    error::out() << ", ";
	}
    }
    if (type->hasVarg()) {
	error::out() << ", ...";
    }
    error::out() << ")";
    if (!type->getRetType()->isVoid()) {
	error::out() << ":" << type->getRetType();
    }
    error::out() << ";" << std::endl << std::endl;
}

void
AstFuncDecl::codegen()
{
    gen::fnDecl(fnIdent.val.c_str(), type, externFlag);
}

/*
 * AstFunctionDef
 */
AstFuncDef::AstFuncDef(Token fnIdent, const Type *type,
		       std::vector<Token> paramToken, bool externFlag)
    : fnIdent{fnIdent}, type{type}, paramToken{paramToken}, body{}
    , externFlag{externFlag}
{
    auto sym = Symtab::addDecl(fnIdent.loc, fnIdent.val, type);
    sym->setDefinitionFlag();

    Symtab::openScope();
    const auto paramType = type->getArgType();
    assert(paramToken.size() == paramType.size());
    for (std::size_t i = 0; i < paramToken.size(); ++i) {
	if (paramToken[i].val.c_str()) {
	    auto sym = Symtab::addDecl(paramToken[i].loc, paramToken[i].val,
				       paramType[i]);
	    sym->setDefinitionFlag();
	    paramIdent.push_back(sym->getInternalIdent().c_str());
	}
    }
}

AstFuncDef::~AstFuncDef()
{
    if (!body) {
	Symtab::closeScope();
    }
}

void
AstFuncDef::appendBody(AstPtr &&body_)
{
    body = std::move(body_);
    Symtab::closeScope();

    auto retType = type->getRetType();
    auto setReturnType = [=](Ast *ast) -> bool {
	if (auto astReturn = dynamic_cast<AstReturn *>(ast)) {
	    astReturn->retType = retType;
	}
	return true;
    };
    body->apply(setReturnType);
}

void
AstFuncDef::print(int indent) const
{
    error::out(indent) << "fn " << fnIdent.val.c_str() << "(";
    for (std::size_t i = 0; i < type->getArgType().size(); ++i) {
	if (paramToken[i].val.c_str()) {
	    error::out() << paramToken[i].val.c_str();
	}
	error::out() << ": " << type->getArgType()[i];
	if (i + 1 < type->getArgType().size()) {
	    error::out() << ", ";
	}
    }
    if (type->hasVarg()) {
	error::out() << ", ...";
    }
    error::out() << ")";
    if (!type->getRetType()->isVoid()) {
	error::out() << ":" << type->getRetType();
    }
    error::out() << std::endl;
    body->print(indent);
    error::out() << std::endl;
}

void
AstFuncDef::codegen()
{
    gen::fnDef(fnIdent.val.c_str(), type, paramIdent, externFlag);
    body->codegen();
    gen::fnDefEnd();
}