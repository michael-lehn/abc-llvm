#include <iostream>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "parser.hpp"

int
main()
{
    abc::lexer::openInputfile(nullptr);
    abc::lexer::init();
    gen::init();

    if (auto ast = abc::parser()) {
	ast->print();
	ast->codegen();
	std::cerr << "generating hello.bc\n";
	gen::print("hello");
    }
}
