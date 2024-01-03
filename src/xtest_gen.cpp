#include <cstdio>

#include "gen.hpp"
#include "types.hpp"

int
main(void)
{
    std::printf("testing gen\n");

    auto ret = Type::getUnsignedInteger(64);
    std::vector<const Type *>	arg{2};
    arg[0] = Type::getUnsignedInteger(64);
    arg[1] = Type::getUnsignedInteger(64);
    auto fn = Type::getFunction(ret, arg);

    std::vector<const char *>	param{2};
    param[0] = "a";
    param[1] = "b";
    gen::fnDef("foo", fn, param);

    auto a = gen::fetch("a", Type::getUnsignedInteger(64));
    auto b = gen::fetch("b", Type::getUnsignedInteger(64));
    auto r = gen::op2r(gen::Add, a, b);

    gen::ret(r);

    gen::dump();
}
