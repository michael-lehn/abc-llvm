#include <iostream>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "parser.hpp"

int
main()
{
    const char *name = "hello";

    abc::lexer::openInputfile(nullptr);
    abc::lexer::init();
    gen::init(name);

    if (auto ast = abc::parser()) {
	ast->print();
	ast->codegen();
	std::cerr << "generating " << name << ".bc\n";
	gen::print(name);
    }
}
