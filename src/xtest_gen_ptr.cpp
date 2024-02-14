#include <iostream>

#include "llvm/IR/Constants.h"

#include "gen.hpp"
#include "expr.hpp"
#include "type.hpp"

int
main(void)
{
    auto tyInt8 = Type::getUnsignedInteger(8);
    auto tyInt = Type::getUnsignedInteger(64);
    auto tyPtrInt = Type::getPointer(tyInt8);

    gen::defStringLiteral("msg0", "hello world!\n", true);
    
    auto ret = tyPtrInt;
    std::vector<const Type *>	arg{1};
    arg[0] = tyInt;
    auto fn = Type::getFunction(ret, arg);

    std::vector<const char *>	param{1};
    param[0] = "a";
    gen::fnDef("foo", fn, param, false);


    auto ptr = gen::loadAddr("msg0");
    auto offset = gen::loadIntConst("2", Type::getUnsignedInteger(16), 10);
    auto r = gen::ptrInc(tyInt, ptr, offset);
    gen::defStringLiteral("msg1", "hello world!\n", true);
    gen::ret(r);
    gen::fnDefEnd();

    std::cerr << "writing to 'ex_gen_ptr.bc'" << std::endl;
    gen::dump_bc("ex_gen_ptr");
}
