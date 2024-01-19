#include "expr.hpp"
#include "symtab.hpp"
#include "gen.hpp"
#include "type.hpp"

int
main(void)
{
    /*
    auto ident = makeIdentifierExpr("foo");
    auto lit = makeLiteralExpr("42");
    auto unary = makeUnaryExpr(BinaryExprKind::UNARY_MINUS, std::move(ident));
    auto binary = createBinaryExpr(
			BinaryExprKind::ADD,
			std::move(unary),
			std::move(lit));
			*/

    Symtab::addDecl(Token::Loc{}, "a", Type::getUnsignedInteger(64))
	->setDefinitionFlag();
    Symtab::addDecl(Token::Loc{}, "b", Type::getUnsignedInteger(64))
	->setDefinitionFlag();


    auto lit42 = Expr::createLiteral("42", 10, nullptr);
    auto a = Expr::createIdentifier("a");
    auto b = Expr::createIdentifier("b");
    auto binary = Expr::createBinary(
			Binary::Kind::ADD,
			Expr::createBinary(
			    Binary::Kind::SUB,
			    std::move(lit42),
			    std::move(a)),
			Expr::createUnaryMinus(
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

    auto r = binary->loadValue();
    gen::ret(r);
    gen::fnDefEnd();


    gen::dump_bc("expr");
    gen::dump_asm("expr0", 0);
    gen::dump_asm("expr1", 1);
    gen::dump_asm("expr2", 2);
    gen::dump_asm("expr3", 3);
}
