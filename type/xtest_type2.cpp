#include <iostream>

#include "integertype.hpp"
#include "pointertype.hpp"

int
main()
{
    auto ty1 = abc::IntegerType::createBool();
    auto ty2 = abc::IntegerType::createBool();

    std::cerr << "ty1 = " << ty1 << ", (void *)ty1 = " << (void *)ty1 << "\n";
    std::cerr << "ty2 = " << ty2 << ", (void *)ty2 = " << (void *)ty2 << "\n";

    auto ptrTy1 = abc::PointerType::create(ty1);
    auto ptrTy2 = abc::PointerType::create(ty2);
    std::cerr << "ptrTy1 = " << ptrTy1
              << ", (void *)ptrTy1 = " << (void *)ptrTy1 << "\n";
    std::cerr << "ptrTy2 = " << ptrTy2
              << ", (void *)ptrTy2 = " << (void *)ptrTy2 << "\n";
}
