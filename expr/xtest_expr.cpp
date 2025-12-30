#include <iostream>

#include "gen/gen.hpp"
#include "type/integertype.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "integerliteral.hpp"
#include "nullptr.hpp"

int
main()
{
    gen::init();

    using namespace abc;

    auto intExpr = IntegerLiteral::create(-1);

    std::cerr << intExpr << "\n";
    std::cerr << "signed val   = " << intExpr->getSignedIntValue() << "\n";
    std::cerr << "unsigned val = " << intExpr->getUnsignedIntValue() << "\n";

    auto idExpr = Identifier::create(UStr::create("a"), UStr::create("a"),
                                     IntegerType::createSigned(32));
    std::cerr << idExpr << "\n";

    auto addExpr = BinaryExpr::create(BinaryExpr::ADD, std::move(intExpr),
                                      std::move(idExpr));
    std::cerr << addExpr << "\n";

    auto np = Nullptr::create();
    std::cerr << "np = " << np << "\n";
}
