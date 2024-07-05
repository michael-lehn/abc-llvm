#include <filesystem>
#include <iostream>

#include "lexer/error.hpp"
#include "lexer/lexer.hpp"
#include "lexer/reader.hpp"
#include "symtab/symtab.hpp"

#include "defaulttype.hpp"
#include "parsetype.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
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
    abc::lexer::getToken();
    gen::init(infile.stem().c_str());
    abc::Symtab newScope;
    abc::initDefaultType();

    while (auto astType = abc::parseType_(true)) {
	astType->print();
	std::cerr << "\n";
    }
    if (abc::lexer::token.kind != abc::lexer::TokenKind::EOI) {
	abc::error::location(abc::lexer::token.loc);
	abc::error::out() << abc::error::setColor(abc::error::BOLD)
	    << abc::lexer::token.loc << ": "
	    << abc::error::setColor(abc::error::BOLD_RED) << "error: "
	    << abc::error::setColor(abc::error::BOLD)
	    << "type expected\n"
	    << abc::error::setColor(abc::error::NORMAL);
	abc::error::fatal();
    }
}
