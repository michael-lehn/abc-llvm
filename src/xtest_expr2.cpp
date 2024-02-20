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
    
    memIdent.push_back(UStr{"first"});
    memType.push_back(Type::getUnsignedInteger(16));

    memIdent.push_back(UStr{"second"});
    memType.push_back(Type::getUnsignedInteger(64));

    auto ty = Type::createIncompleteStruct("Foo");
    return ty->complete(std::move(memIdent), std::move(memType));
}

int
main(void)
{
    Symtab::addDecl(Token::Loc{}, UStr{"foo"}, makeSomeStructType());
    auto foo = Identifier::create(UStr{"foo"}, Token::Loc{});

    auto foo_second = BinaryExpr::createMember(std::move(foo), UStr{"first"});

    foo_second->print();

}
