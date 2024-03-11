#include <filesystem>
#include <iostream>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "lexer/reader.hpp"
#include "parser.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -Idir... ] "
	<< "infile" << std::endl;
    std::exit(1);
}

int
main(int argc, char *argv[])
{
    std::filesystem::path infile;
    std::filesystem::path outfile;

    for (int i = 1; i < argc; ++i) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case 'o':
		    if (outfile.empty() && !argv[i][2] && i + 1 < argc) {
			outfile = argv[i + 1];
			++i;
		    } else if (outfile.empty() && argv[i][2]) {
			outfile = &argv[i][2];
		    } else {
			usage(argv[0]);
		    }
		    break;
		case 'I':
		    if (!argv[i][2] && i + 1 < argc) {
			abc::lexer::addSearchPath(argv[i + 1]);
			++i;
		    } else if (argv[i][2]) {
			abc::lexer::addSearchPath(&argv[i][2]);
		    } else {
			usage(argv[0]);
		    }
		    break;
		default:
		    usage(argv[0]);
	    }
	} else {
	    if (infile.empty()) {
		infile = argv[i];
	    } else {
		usage(argv[0]);
	    }
	}
    }
    if (infile.empty()) {
	usage(argv[0]);
    }
    if (outfile.empty()) {
	outfile = infile.filename().replace_extension("bc");
    }


    if (!abc::lexer::openInputfile(infile.c_str())) {
	std::cerr << argv[0] << ": error: can not open '" << infile.c_str()
	    << "'\n";
	return 1;
    }
    abc::lexer::init();
    gen::init(infile.stem().c_str());

    if (auto ast = abc::parser()) {
	ast->print();
	ast->codegen();
	std::cerr << "generating " << outfile.c_str() << "\n";
	gen::print(outfile.c_str());
    }
}
