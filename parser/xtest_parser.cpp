#include "gen/gen.hpp"
#include "parser.hpp"

int
main()
{
    gen::init();

    auto ast = abc::parser();
    ast->print();
}
