#include "expr.hpp"
#include "constexpr.hpp"

int
main(void)
{
    auto aTy = Type::getUnsignedInteger(64);
    aTy = Type::getArray(aTy, 5);

    ConstExpr	constExpr(aTy);

    constExpr.add(Expr::createLiteral("1"));
    constExpr.add(Expr::createLiteral("2"));
    constExpr.add(Expr::createLiteral("3"));

    constExpr.print();
    auto aVal = constExpr.load();

    Symtab::addDecl(Token::Loc{}, "a", aTy);
    auto a = Expr::createIdentifier("a");
    gen::defGlobal("a", aTy, aVal);

    // --------
    auto b1Ty = Type::getUnsignedInteger(64);
    auto b2Ty = Type::getUnsignedInteger(16);
    auto bTy = Type::createIncompleteStruct("B");
    bTy->complete({"first", "second"}, {b1Ty, b2Ty});

    ConstExpr	constExpr2(bTy);
    constExpr2.add(Expr::createLiteral("11"));
    constExpr2.add(Expr::createLiteral("21"));

    constExpr2.print();
    auto bVal = constExpr2.load();

    Symtab::addDecl(Token::Loc{}, "b", bTy);
    auto b = Expr::createIdentifier("b");
    gen::defGlobal("b", bTy, bVal);

    // --------

    constExpr.add(std::move(constExpr2));

    gen::dump_bc("constexpr");
}
