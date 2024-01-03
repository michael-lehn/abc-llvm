#include <iostream>

#include "gen.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int
main(void)
{
    getToken();
    parser();

    std::cerr << "generated code:" << std::endl;
    gen::dump();
}
