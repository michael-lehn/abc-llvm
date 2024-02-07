#include <cstdio>
#include <iostream>

#include "expr.hpp"

int
main(void)
{
    auto zero = Expr::createIntegerLiteral("0", 10);
    auto larger = Expr::createIntegerLiteral("1234", 10);


    std::cout << "typeof(zero) = " << zero->getType() << std::endl;
    std::cout << "typeof(larger) = " << larger->getType() << std::endl;

    auto sum = Expr::createBinary(Binary::Kind::ADD,
				  std::move(zero),
				  std::move(larger));
    std::cout << "typeof(sum) = " << sum->getType() << std::endl;
}
