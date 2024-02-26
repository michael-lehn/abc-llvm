#include "binaryexpr.hpp"
#include "expr.hpp"
#include "gen.hpp"
#include "identifier.hpp"
#include "symtab.hpp"
#include "type.hpp"

const Type *
makeSomeStructType(void)
{
    std::vector<UStr> memIdent;
    std::vector<const Type *> memType;
    
    memIdent.push_back(UStr::create("first"));
    memType.push_back(Type::getUnsignedInteger(16));

    memIdent.push_back(UStr::create("second"));
    memType.push_back(Type::getUnsignedInteger(64));

    auto ty = Type::createIncompleteStruct(UStr::create("Foo"));
    return ty->complete(std::move(memIdent), std::move(memType));
}

int
main(void)
{
    gen::init();
    Symtab::openScope();

    Symtab::addDecl(Token::Loc{}, UStr::create("foo"), makeSomeStructType());
    auto foo = Identifier::create(UStr::create("foo"), Token::Loc{});

    auto foo_second = BinaryExpr::createMember(std::move(foo),
					       UStr::create("first"));

    foo_second->print();

    Symtab::closeScope();
}
