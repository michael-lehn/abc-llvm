#include <cstdio>
#include <iostream>

#include "type.hpp"

int
main(void)
{
    auto v = Type::getVoid();
    std::cout << "v = " << v << ", addr = " << (int *)v << std::endl;

    auto t1 = Type::getConst(Type::getPointer(Type::getUnsignedInteger(16)));
    auto t2 = Type::getPointer(Type::getUnsignedInteger(16));
    auto t3 = Type::getConstRemoved(t2);
    auto t4 = Type::getPointer(t1);
    auto t5 = Type::getPointer(t1);
    auto t6 = Type::getPointer(t2);
    auto t7 = Type::getPointer(t3);

    std::cout << "t1 = " << t1 << ", addr = " << (int *)t1 << std::endl;
    std::cout << "t2 = " << t2 << ", addr = " << (int *)t2 << std::endl;
    std::cout << "t3 = " << t3 << ", addr = " << (int *)t3 << std::endl;
    std::cout << "t4 = " << t3 << ", addr = " << (int *)t4 << std::endl;
    std::cout << "t5 = " << t3 << ", addr = " << (int *)t5 << std::endl;
    std::cout << "t6 = " << t3 << ", addr = " << (int *)t6 << std::endl;
    std::cout << "t7 = " << t3 << ", addr = " << (int *)t7 << std::endl;



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
