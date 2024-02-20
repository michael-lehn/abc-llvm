#include <cstdio>
#include <iostream>

#include "integerliteral.hpp"
#include "binaryexpr.hpp"

int
main(void)
{
    auto zero = IntegerLiteral::create("0");
    auto larger = IntegerLiteral::create("1234");


    std::cout << "typeof(zero) = " << zero->type << std::endl;
    std::cout << "typeof(larger) = " << larger->type << std::endl;

    auto sum = BinaryExpr::create(BinaryExpr::Kind::ADD,
				  std::move(zero),
				  std::move(larger));
    std::cout << "typeof(sum) = " << sum->type << std::endl;
}
