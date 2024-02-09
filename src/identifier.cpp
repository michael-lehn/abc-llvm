#include <iostream>
#include <iomanip>

#include "error.hpp"
#include "gen.hpp"
#include "identifier.hpp"
#include "proxyexpr.hpp"
#include "symtab.hpp"

Identifier::Identifier(UStr ident, const Type *type, Token::Loc loc)
    : Expr{loc, type}, ident{ident}
{
}


ExprPtr
Identifier::create(UStr ident, Token::Loc loc)
{
    auto symEntry = Symtab::get(ident);
    if (!symEntry) {
	error::out() << loc << " undeclared identifier '"
	    << ident.c_str() << "'" << std::endl;
	error::fatal();
    } else if (symEntry->holdsExpr()) {
	return ProxyExpr::create(symEntry->getExpr(), loc);
    }
    auto type = symEntry->getType();
    auto p = new Identifier{ident, type, loc};
    return std::unique_ptr<Identifier>{p};
}

bool
Identifier::hasAddr() const
{
    return true;
}

bool
Identifier::isLValue() const
{
    return true;
}

bool
Identifier::isConst() const
{
    return false;
}

// for code generation
gen::ConstVal
Identifier::loadConstValue() const
{
    assert(0);
    return nullptr;
}

gen::Reg
Identifier::loadValue() const
{
    if (type->isFunction()) {
	return loadAddr();
    }
    return gen::fetch(ident.c_str(), type);
}

gen::Reg
Identifier::loadAddr() const
{
    return gen::loadAddr(ident.c_str());
}

void
Identifier::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
Identifier::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << ident.c_str() << " [ " << type << " ] " << std::endl;
}
    

