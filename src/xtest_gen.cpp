#include <iostream>

#include "gen.hpp"
#include "types.hpp"

int
main(void)
{
    auto ret = Type::getUnsignedInteger(64);
    std::vector<const Type *>	arg{2};
    arg[0] = Type::getUnsignedInteger(64);
    arg[1] = Type::getUnsignedInteger(64);
    auto fn = Type::getFunction(ret, arg);

    std::vector<const char *>	param{2};
    param[0] = "a";
    param[1] = "b";
    gen::fnDef("foo", fn, param);

    auto thenLabel = gen::getLabel("then");
    auto elseLabel = gen::getLabel("else");
    auto endLabel = gen::getLabel("end");

    auto a = gen::fetch("a", Type::getUnsignedInteger(64));
    auto zero = gen::loadConst("0", Type::getUnsignedInteger(64));
    auto cond = gen::cond(gen::EQ, a, zero);
    gen::jmp(cond, thenLabel, elseLabel);

    gen::labelDef(thenLabel);
    {
	auto a = gen::fetch("a", Type::getUnsignedInteger(64));
	auto b = gen::fetch("b", Type::getUnsignedInteger(64));
	auto t = gen::aluInstr(gen::ADD, a, b);
	gen::store(t, "a", Type::getUnsignedInteger(64));
	gen::jmp(endLabel);
    }
    gen::labelDef(elseLabel);
    {
	auto a = gen::fetch("a", Type::getUnsignedInteger(64));
	auto b = gen::fetch("b", Type::getUnsignedInteger(64));
	auto t = gen::aluInstr(gen::SUB, a, b);
	gen::store(t, "a", Type::getUnsignedInteger(64));
	gen::jmp(endLabel);
    }
    gen::labelDef(endLabel);

    auto r = gen::fetch("a", Type::getUnsignedInteger(64));
    gen::ret(r);
    //gen::fnDefEnd();

    std::cerr << "writing to 'ex_gen.bc'" << std::endl;
    gen::dump_bc("ex_gen");
}
