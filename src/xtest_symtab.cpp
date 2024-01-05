#include <cassert>
#include <cstdio>
#include <iostream>

#include "symtab.hpp"
#include "types.hpp"
#include "ustr.hpp"

void
check(UStr ident)
{

    printf("check: %s identifier '%s'\n",
	   symtab::get(ident) != nullptr ? "known" : "unkown",
	   ident.c_str());
}

void
add(UStr ident)
{
    if (symtab::get(ident, symtab::CurrentScope)) {
	printf("add: identifier '%s' already declared!\n", ident.c_str());
    } else {
	Token::Loc loc = {{1, 2}, {1, 4}};
	auto t = Type::getUnsignedInteger(16);

	assert(symtab::add(loc, ident, t));

	printf("add: identifier '%s' declared\n", ident.c_str());
    }
}

void
addToRootScope(UStr ident)
{
    if (symtab::get(ident, symtab::RootScope)) {
	printf("add: identifier '%s' already declared in root scope!\n",
	       ident.c_str());
    } else {
	Token::Loc loc = {{1, 2}, {1, 4}};
	auto t = Type::getUnsignedInteger(16);

	assert(symtab::addToRootScope(loc, ident, t));

	printf("add: identifier '%s' declared\n", ident.c_str());
    }
}


int
main(void)
{
    addToRootScope("A");

    symtab::openScope();
    add("a");
    symtab::print(std::cout);
    symtab::closeScope();
    symtab::print(std::cout);

    addToRootScope("b");
    addToRootScope("x");
    symtab::print(std::cout);

    symtab::openScope();
    add("a");
    add("b");
    addToRootScope("X");
    symtab::print(std::cout);
    symtab::closeScope();


    symtab::openScope();
    add("c");
    symtab::print(std::cout);
    symtab::closeScope();
}
