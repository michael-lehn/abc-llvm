#include "expr.hpp"
#include "gen.hpp"
#include "types.hpp"

int
main(void)
{
    /*
    auto ident = makeIdentifierExpr("foo");
    auto lit = makeLiteralExpr("42");
    auto unary = makeUnaryExpr(BinaryExprKind::UNARY_MINUS, std::move(ident));
    auto binary = getBinaryExpr(
			BinaryExprKind::ADD,
			std::move(unary),
			std::move(lit));
			*/
    auto lit42 = Expr::getLiteral("42");
    auto a = Expr::getIdentifier("a");
    auto b = Expr::getIdentifier("b");
    auto binary = Expr::getBinary(
			Binary::Kind::ADD,
			Expr::getBinary(
			    Binary::Kind::SUB,
			    std::move(lit42),
			    std::move(a)),
			Expr::getUnaryMinus(
			    std::move(b)));

    binary->print();


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

    auto r = binary->load();
    gen::ret(r);
    gen::fnDefEnd();


    gen::dump_bc("expr");
    gen::dump_asm("expr0", 0);
    gen::dump_asm("expr1", 1);
    gen::dump_asm("expr2", 2);
    gen::dump_asm("expr3", 3);
    
}
