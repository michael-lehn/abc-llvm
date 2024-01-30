#include "expr.hpp"
#include "initializerlist.hpp"

int
main(void)
{
    auto aTy = Type::getUnsignedInteger(64);
    aTy = Type::getArray(aTy, 5);

    //InitializerList initList(aTy);
    InitializerList initList;

    initList.add(Expr::createLiteral("1"));
    initList.add(Expr::createLiteral("2"));
    initList.add(Expr::createLiteral("3"));

    initList.print();
    initList.setType(aTy);
    auto aVal = initList.loadConst();

    Symtab::addDecl(Token::Loc{}, "a", aTy);
    auto a = Expr::createIdentifier("a");
    gen::defGlobal("a", aTy, aVal);

    // --------
    auto b1Ty = Type::getUnsignedInteger(64);
    auto b2Ty = Type::getUnsignedInteger(16);
    auto bTy = Type::createIncompleteStruct("B");
    bTy->complete({"first", "second"}, {b1Ty, b2Ty});

    InitializerList initList2(bTy);
    initList2.add(Expr::createLiteral("11"));
    initList2.add(Expr::createLiteral("21"));

    initList2.print();
    auto bVal = initList2.loadConst();

    Symtab::addDecl(Token::Loc{}, "b", bTy);
    auto b = Expr::createIdentifier("b");
    gen::defGlobal("b", bTy, bVal);

    // --------

    initList.add(std::move(initList2));

    gen::dump_bc("constexpr");
}
