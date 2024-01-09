#include <cstdio>
#include <iostream>

#include "type.hpp"

int
main(void)
{
    auto t1 = Type::getUnsignedInteger(16);
    auto t2 = Type::getUnsignedInteger(16);
    auto t3 = Type::getUnsignedInteger(32);
    auto t4 = Type::getPointer(t1);
    auto t5 = Type::getPointer(t1);
    auto t6 = Type::getPointer(t2);
    auto t7 = Type::getPointer(t3);

    std::cout << "t1 = " << t1 << std::endl;

    std::printf("u16 t1 = %p\n", t1);
    std::printf("u16 t2 = %p\n", t2);

    std::printf("u32 t3 = %p\n", t3);

    std::printf("-> u16 t4 = %p\n", t4);
    std::printf("-> u16 t5 = %p\n", t5);

    std::printf("-> u16 t6 = %p\n", t6);
    std::printf("-> u32 t7 = %p\n", t7);

    auto ret = Type::getUnsignedInteger(16);
    std::vector<const Type *>	arg{3};
    arg[0] = Type::getUnsignedInteger(16);
    arg[1] = Type::getUnsignedInteger(64);
    arg[2] = Type::getUnsignedInteger(8);

    auto fn = Type::getFunction(ret, arg);

    std::printf("-> fn = %p\n", fn);

    arg[2] = Type::getUnsignedInteger(16);
    auto fn2 = Type::getFunction(ret, arg);

    std::printf("-> fn2 = %p\n", fn2);
    std::cout << "fn2 = " << fn2 << std::endl;
}
