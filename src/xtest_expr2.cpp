#include "expr.hpp"
#include "symtab.hpp"
#include "gen.hpp"
#include "type.hpp"

const Type *
makeSomeStructType(void)
{
    std::vector<const char *> memIdent;
    std::vector<const Type *> memType;
    
    memIdent.push_back(UStr{"first"}.c_str());
    memType.push_back(Type::getUnsignedInteger(16));

    memIdent.push_back(UStr{"second"}.c_str());
    memType.push_back(Type::getUnsignedInteger(64));

    auto ty = Type::createIncompleteStruct("Foo");
    return ty->complete(std::move(memIdent), std::move(memType));
}

int
main(void)
{
    Symtab::addDecl(Token::Loc{}, UStr{"foo"}, makeSomeStructType());
    auto foo = Expr::createIdentifier(UStr{"foo"}, Token::Loc{});

    auto foo_second = Expr::createMember(std::move(foo), UStr{"first"});

    foo_second->print();

}
