#include <iomanip>
#include <iostream>
#include <sstream>

#include "compoundliteral.hpp"
#include "error.hpp"
#include "identifier.hpp"
#include "symtab.hpp"

CompoundLiteral::CompoundLiteral(AstInitializerListPtr &&ast, Token::Loc loc)
    : Expr{loc, ast->type}, ast{std::move(ast)}
    , initializerList{this->ast->createInitializerList()}
{
    static std::size_t id;
    std::stringstream ss;
    ss << ".tmp.compound_literal." << id++;
    genIdent = UStr::create(ss.str());
}

bool
CompoundLiteral::createTmp() const
{
    if (gen::defLocal(genIdent.c_str(), type)) {
	initializerList.store(loadAddr());
	return true;
    }
    return false;
}

ExprPtr
CompoundLiteral::create(AstInitializerListPtr &&ast, Token::Loc loc)
{
    if (!ast->type) {
	error::out() << loc << "error: initializer has no type" << std::endl;
	error::fatal();
	return nullptr;
    }
    auto p = new CompoundLiteral{std::move(ast), loc};
    return std::unique_ptr<CompoundLiteral>{p};
}

bool
CompoundLiteral::hasAddr() const
{
    return true;
}

bool
CompoundLiteral::isLValue() const
{
    return false;
}

bool
CompoundLiteral::isConst() const
{
    return initializerList.isConst();
}

// for code generation
gen::ConstVal
CompoundLiteral::loadConstValue() const
{
    return initializerList.loadConstValue();
}

gen::Reg
CompoundLiteral::loadValue() const
{
    createTmp();
    return gen::fetch(genIdent.c_str(), type);
}

gen::Reg
CompoundLiteral::loadAddr() const
{
    if (createTmp()) {
	std::cerr << "CompoundLiteral: loadAddr() called before loadValue()\n";
	std::cerr << "Writing to a temporary?\n";
    }
    return gen::loadAddr(genIdent.c_str());
}

void
CompoundLiteral::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
CompoundLiteral::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    std::cerr << "(" << type << ")" << std::endl;
    initializerList.print(indent + 4);
}

void
CompoundLiteral::printFlat(std::ostream &out, int prec) const
{
    out << "(" << type << ")";
    initializerList.printFlat(out, prec);
}
