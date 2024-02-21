#include <iostream>
#include <iomanip>

#include "error.hpp"
#include "gen.hpp"
#include "identifier.hpp"
#include "proxyexpr.hpp"
#include "symtab.hpp"

Identifier::Identifier(UStr ident, ExprPtr expr)
    : Expr{expr->loc, expr->type}, expr{std::move(expr)}
    , ident{ident}, identUser{ident}, misusedAsMember{false}
{
}

Identifier::Identifier(UStr ident, UStr identUser, const Type *type,
		       Token::Loc loc, bool misusedAsMember)
    : Expr{loc, type}, ident{ident}, identUser{identUser}
    , misusedAsMember{misusedAsMember}
{
}

ExprPtr
Identifier::create(UStr ident, Token::Loc loc)
{
    auto sym = Symtab::get(ident);
    if (!sym) {
	error::out() << loc << " undeclared identifier '"
	    << ident.c_str() << "'" << std::endl;
	error::fatal();
	return nullptr;
    } else if (sym->holdsExpr()) {
	auto p = new Identifier{ident, ProxyExpr::create(sym->expr())};
	return std::unique_ptr<Identifier>{p};
    } else {
	auto p = new Identifier{sym->getInternalIdent(), ident, sym->type(),
				loc};
	return std::unique_ptr<Identifier>{p};
    }
}

ExprPtr
Identifier::create(UStr ident, const Type *type, Token::Loc loc)
{
    assert(type);
    auto p = new Identifier{ident, ident, type, loc, true};
    return std::unique_ptr<Identifier>{p};
}

bool
Identifier::hasAddr() const
{
    assert(!misusedAsMember);
    assert(type);
    if (expr) {
	return false;
    } else if (!type) {
	return false;
    } else {
	return type->hasSize();
    }
}

bool
Identifier::isLValue() const
{
    assert(!misusedAsMember);
    return !expr;
}

bool
Identifier::isConst() const
{
    return !expr || expr->isConst();
}

// for code generation
gen::ConstVal
Identifier::loadConstValue() const
{
    assert(isConst());
    assert(!misusedAsMember);
    assert(expr);
    return expr->loadConstValue();
}

gen::Reg
Identifier::loadValue() const
{
    assert(!misusedAsMember);
    assert(type);
    if (expr) {
	return expr->loadValue();
    }
    if (type->isFunction()) {
	return loadAddr();
    }
    assert(ident.c_str());
    return gen::fetch(ident.c_str(), type);
}

gen::Reg
Identifier::loadAddr() const
{
    assert(!misusedAsMember);
    assert(hasAddr());
    assert(ident.c_str());
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
    std::cerr << identUser.c_str() << " [ " << type << " ] " << std::endl;
}
    
void
Identifier::printFlat(std::ostream &out, int prec) const
{
    out << identUser.c_str();
}

