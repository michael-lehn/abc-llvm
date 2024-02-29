#include <iostream>

#include "lexer/loc.hpp"
#include "type/integertype.hpp"

#include "symtab.hpp"

void
add(const char *name)
{
    using namespace abc;
    auto addDecl = Symtab::addDeclaration(lexer::Loc{},
					  UStr::create(name),
					  IntegerType::createSigned(32));

    std::cerr << "add: name = " << name << "\n";
    std::cerr << "added = " << addDecl.second << "\n";
    std::cerr << "id = " << addDecl.first->id << "\n";
    std::cerr << "type = " << addDecl.first->type << "\n";
    std::cerr << "\n";
}

int
main()
{
    abc::Symtab guard;
    add("foo");
    add("foo");
    {
	abc::Symtab guard(abc::UStr::create("bar"));
	add("foo");
	add("foo");
    }
}
