#include "integerliteral.hpp"
#include "symtab.hpp"
#include "identifier.hpp"
#include "initializerlist.hpp"

int
main(void)
{
    auto aTy = Type::getUnsignedInteger(64);
    aTy = Type::getArray(aTy, 5);

    //InitializerList initList(aTy);
    InitializerList initList;

    initList.add(IntegerLiteral::create("1"));
    initList.add(IntegerLiteral::create("2"));
    initList.add(IntegerLiteral::create("3"));

    initList.print();
    initList.setType(aTy);
    auto aVal = initList.loadConstValue();

    Symtab::addDecl(Token::Loc{}, "a", aTy);
    auto a = Identifier::create("a");
    gen::defGlobal("a", aTy, aVal);

    // --------
    auto b1Ty = Type::getUnsignedInteger(64);
    auto b2Ty = Type::getUnsignedInteger(16);
    auto bTy = Type::createIncompleteStruct("B");
    bTy->complete({"first", "second"}, {b1Ty, b2Ty});

    InitializerList initList2(bTy);
    initList2.add(IntegerLiteral::create("11"));
    initList2.add(IntegerLiteral::create("21"));

    initList2.print();
    auto bVal = initList2.loadConstValue();

    Symtab::addDecl(Token::Loc{}, "b", bTy);
    auto b = Identifier::create("b");
    gen::defGlobal("b", bTy, bVal);

    // --------

    initList.add(std::move(initList2));

    gen::dump_bc("constexpr");
}
