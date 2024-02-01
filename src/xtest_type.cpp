#include <cstdio>
#include <iostream>

#include "symtab.hpp"
#include "type.hpp"

void
printType(const Type *ty, const char *str)
{
    std::cout << str << " = " << ty << ", addr = " << (void *)ty << std::endl;
}

int
main(void)
{
    /*
     * primitive type i32
     */
    auto ty_i32 = Type::getSignedInteger(16);
    printType(ty_i32, "ty_i32");

    /*
     * type int: i32;
     */
    auto ty_int = Type::createAlias("int", ty_i32);
    printType(ty_int, "ty_int");
    if (!Symtab::addTypeAlias("int", ty_i32)) {
	assert(0 && "type already defined");
    }
}
