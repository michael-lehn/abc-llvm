#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "gen.hpp"
#include "lexer.hpp"
#include "parser.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -S | -B] "
	<< "[ -Olevel ] "
	<< "infile" << std::endl;
    std::exit(1);
}

int
main(int argc, char *argv[])
{
    const char *infile = nullptr;
    enum Output { ASM = 1, BC = 2 } output = ASM;
    int optLevel = 0;

    for (int i = 1; i < argc; ++i) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case 'S':
		    output = ASM;
		    break;
		case 'B':
		    output = BC;
		    break;
		case 'O':
		    optLevel = argv[i][2] - '0';
		    if (optLevel < 0 || optLevel > 3) {
			usage(argv[0]);
		    }
		    break;
		default:
		    usage(argv[0]);
	    }
	} else if (infile) {
	    usage(argv[0]);
	} else {
	    infile = argv[i];
	}
    }
    if (!infile) {
	usage(argv[0]);
    }

    if (!setLexerInputfile(infile)) {
	std::cerr << "can not read '" << argv[1] << "'" << std::endl;
    }

    gen::setTarget(optLevel);
    parser();

    if (output == ASM) {
	gen::dump_asm(std::filesystem::path(infile).stem().c_str(), optLevel);
    } else {
	gen::dump_bc(std::filesystem::path(infile).stem().c_str());
    }
}
