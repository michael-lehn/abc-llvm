#include "gen.hpp"
#include "integerliteral.hpp"
#include "symtab.hpp"
#include "identifier.hpp"
#include "initializerlist.hpp"

int
main(void)
{
    gen::init();
    Symtab::openScope();

    auto aTy = Type::getUnsignedInteger(64);
    aTy = Type::getArray(aTy, 5);

    InitializerList initList(aTy);

    initList.add(IntegerLiteral::create(1));
    initList.add(IntegerLiteral::create(2));
    initList.add(IntegerLiteral::create(3));

    initList.print();
    auto aVal = initList.loadConstValue();

    Symtab::addDecl(Token::Loc{}, UStr::create("a"), aTy);
    auto a = Identifier::create(UStr::create("a"));
    gen::defGlobal("a", aTy, aVal);

    // --------
    auto b1Ty = Type::getUnsignedInteger(64);
    auto b2Ty = Type::getUnsignedInteger(16);
    auto bTy = Type::createIncompleteStruct(UStr::create("B"));
    bTy->complete(std::vector<UStr>{UStr::create("first"),
				    UStr::create("second")},
		  std::vector<const Type *>{b1Ty, b2Ty});


    InitializerList initList2(bTy);
    initList2.add(IntegerLiteral::create(11));
    initList2.add(IntegerLiteral::create(21));

    initList2.print();
    auto bVal = initList2.loadConstValue();

    Symtab::addDecl(Token::Loc{}, UStr::create("b"), bTy);
    auto b = Identifier::create(UStr::create("b"));
    gen::defGlobal("b", bTy, bVal);

    // --------

    initList.add(std::move(initList2));

    gen::dump_bc("constexpr");
    Symtab::closeScope();
}
