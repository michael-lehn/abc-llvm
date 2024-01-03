#include "expr.hpp"
#include "gen.hpp"
#include "types.hpp"

int
main(void)
{
    /*
    auto ident = makeIdentifierExpr("foo");
    auto lit = makeLiteralExpr("42");
    auto unary = makeUnaryExpr(ExprKind::UNARY_MINUS, std::move(ident));
    auto binary = makeBinaryExpr(
			ExprKind::ADD,
			std::move(unary),
			std::move(lit));
			*/
    auto lit42 = makeLiteralExpr("42");
    auto a = makeIdentifierExpr("a");
    auto b = makeIdentifierExpr("b");
    auto binary = makeBinaryExpr(
			ExprKind::ADD,
			makeBinaryExpr(
			    ExprKind::SUB,
			    std::move(lit42),
			    std::move(a)),
			makeUnaryMinusExpr(
			    std::move(b)));

    print(binary.get());


    // generate function 'foo(a: u64, b: 64): u64'
    const Type *ret = Type::getUnsignedInteger(64);
    std::vector<const Type *>	arg = {
	Type::getUnsignedInteger(64),
	Type::getUnsignedInteger(64),
    };
    std::vector<const char *>	param = {
	"a",
	"b",
    };
    auto fn = Type::getFunction(ret, arg);
    gen::fnDef("foo", fn, param);

    auto r = load(binary.get());
    gen::ret(r);


    gen::dump_bc("expr");
    gen::dump_asm("expr0", 0);
    gen::dump_asm("expr1", 1);
    gen::dump_asm("expr2", 2);
    gen::dump_asm("expr3", 3);
    
}
