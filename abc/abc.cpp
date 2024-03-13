#include <cstring>
#include <filesystem>
#include <iostream>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "lexer/reader.hpp"
#include "parser/parser.hpp"

void
usage(const char *prog, int exit = 1)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -c | -S | --emit-llvm ] "
	<< "[ --help ] "
	<< "[ --print-ast ] "
	<< "[ -Idir... ] "
	<< "infile" << std::endl;
    std::exit(exit);
}

int
main(int argc, char *argv[])
{
    std::filesystem::path infile;
    std::filesystem::path outfile;
    gen::FileType outputFileType = gen::OBJECT_FILE;
    bool printAst = false;

    for (int i = 1; i < argc; ++i) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case '-':
		    if (!strcmp(argv[i], "--help")) {
			usage(argv[0], 0);
		    } else if (!strcmp(argv[i], "--print-ast")) {
			printAst = true;
		    } else if (!strcmp(argv[i], "--emit-llvm")) {
			outputFileType = gen::LLVM_FILE;
		    }
		    break;
		case 'c':
		    outputFileType = gen::OBJECT_FILE;
		    break;
		case 'S':
		    outputFileType = gen::ASSEMBLY_FILE;
		    break;
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
		    break;
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
	switch (outputFileType) {
	    case gen::ASSEMBLY_FILE:
		outfile = infile.filename().replace_extension("s");
		break;
	    case gen::OBJECT_FILE:
		outfile = infile.filename().replace_extension("o");
		break;
	    case gen::LLVM_FILE:
		outfile = infile.filename().replace_extension("ll");
		break;
	    default:
		assert(0);
		break;
	}
    }


    if (!abc::lexer::openInputfile(infile.c_str())) {
	std::cerr << argv[0] << ": error: can not open '" << infile.c_str()
	    << "'\n";
	return 1;
    }
    abc::lexer::init();
    gen::init(infile.stem().c_str());

    if (auto ast = abc::parser()) {
	if (printAst) {
	    ast->print();
	}
	ast->codegen();
	gen::print(outfile.c_str(), outputFileType);
    } else {
	std::exit(1);
    }
}
