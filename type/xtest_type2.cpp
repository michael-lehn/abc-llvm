#include <iostream>

#include "functiontype.hpp"
#include "integertype.hpp"

int
main()
{
    auto ty1 = abc::IntegerType::createBool();
    auto ty2 = abc::IntegerType::createBool();

    std::cerr << "ty1 = " << ty1 << ", (void *)ty1 = " << (void *)ty1 << "\n";
    std::cerr << "ty2 = " << ty2 << ", (void *)ty2 = " << (void *)ty2 << "\n";
}
