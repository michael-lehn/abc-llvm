#include <iostream>

#include "gen/gen.hpp"
#include "type/floattype.hpp"

#include "binaryexpr.hpp"
#include "identifier.hpp"
#include "floatliteral.hpp"
#include "nullptr.hpp"
#include "unaryexpr.hpp"

int
main()
{
    gen::init();

    using namespace abc;

    auto fltExpr = FloatLiteral::create(UStr::create("1.25"));
    auto unaryExpr = UnaryExpr::create(UnaryExpr::MINUS,
				       std::move(fltExpr));

    std::cerr << unaryExpr << "\n";

    auto idExpr = Identifier::create(UStr::create("a"),
				     UStr::create("a"),
				     unaryExpr->type);
    std::cerr << idExpr << "\n";

    auto addExpr = BinaryExpr::create(BinaryExpr::ADD,
				      std::move(unaryExpr),
				      std::move(idExpr));
    std::cerr << addExpr << "\n";
}
