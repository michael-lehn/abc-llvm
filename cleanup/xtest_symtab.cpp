#include <cassert>
#include <cstdio>
#include <iostream>

#include "symtab.hpp"
#include "type.hpp"
#include "ustr.hpp"

void
check(UStr ident)
{

    printf("check: %s identifier '%s'\n",
	   Symtab::get(ident) != nullptr ? "known" : "unkown",
	   ident.c_str());
}

void
add(UStr ident)
{
    if (Symtab::get(ident, Symtab::CurrentScope)) {
	printf("add: identifier '%s' already declared!\n", ident.c_str());
    } else {
	Token::Loc loc = {{1, 2}, {1, 4}, UStr::create("some_file")};
	auto t = Type::getUnsignedInteger(16);

	assert(Symtab::addDecl(loc, ident, t));

	printf("add: identifier '%s' declared\n", ident.c_str());
    }
}

void
addToRootScope(UStr ident)
{
    if (Symtab::get(ident, Symtab::RootScope)) {
	printf("add: identifier '%s' already declared in root scope!\n",
	       ident.c_str());
    } else {
	Token::Loc loc = {{1, 2}, {1, 4}, UStr::create("some_file")};
	auto t = Type::getUnsignedInteger(16);

	assert(Symtab::addDeclToRootScope(loc, ident, t));

	printf("add: identifier '%s' declared\n", ident.c_str());
    }
}

int
main(void)
{
    gen::init();
    Symtab::openScope();

    addToRootScope(UStr::create("A"));

    Symtab::openScope();
    add(UStr::create("a"));
    Symtab::print(std::cout);
    Symtab::closeScope();
    Symtab::print(std::cout);

    addToRootScope(UStr::create("b"));
    addToRootScope(UStr::create("x"));
    Symtab::print(std::cout);

    Symtab::openScope();
    add(UStr::create("a"));
    add(UStr::create("b"));
    addToRootScope(UStr::create("X"));
    Symtab::print(std::cout);
    Symtab::closeScope();


    Symtab::openScope();
    add(UStr::create("c"));
    Symtab::print(std::cout);
    Symtab::closeScope();

    Symtab::closeScope();
}
