#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "lexer.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -Idir... ] "
	<< "[ infile ]" << std::endl;
    std::exit(1);
}

int
main(int argc, char *argv[])
{
    /*
    std::filesystem::path infile;
    std::filesystem::path outfile;

    for (int i = 1; i < argc; ++i) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case 'o':
		    if (!argv[i][2] && i + 1 < argc) {
			outfile = argv[i + 1];
			++i;
		    } else if (argv[i][2]) {
			outfile = &argv[i][2];
		    } else {
			usage(argv[0]);
		    }
		    break;
		case 'I':
		    if (!argv[i][2] && i + 1 < argc) {
			lexer::addIncludePath(argv[i + 1]);
			++i;
		    } else if (argv[i][2]) {
			lexer::addIncludePath(&argv[i][2]);
		    } else {
			usage(argv[0]);
		    }
		    break;
		default:
		    usage(argv[0]);
	    }
	} else {
	    infile = argv[i];
	}
    }
    infile.empty()
	? lexer::setLexerInputfile(nullptr)
	: lexer::setLexerInputfile(infile.c_str());

    std::ostream *fp = &std::cout;
    std::ofstream fout;
    if (!outfile.empty()) {
	fout.open(outfile.c_str());
	if (!fout) {
	    std::cerr << "can not open " << outfile.c_str() << std::endl;
	    std::exit(1);
	}
	fp = &fout;
    }

    while (lexer::getToken() != lexer::TokenKind::EOI) {
	*fp << lexer::token.loc.path.c_str() << ":"
	    << lexer::token.loc.from.line << "."
	    << lexer::token.loc.from.col << "-"
	    << lexer::token.loc.to.line << "."
	    << lexer::token.loc.to.col << ": "
	    << enumConstCStr(lexer::token.kind) << " "
	    << "'" << lexer::token.kind << "' "
	    << (lexer::token.kind == lexer::TokenKind::STRING_LITERAL
		    ? lexer::token.valRaw.c_str()
		    : lexer::token.val.c_str())
	    << std::endl;
	if (lexer::token.kind == lexer::TokenKind::BAD) {
	    break;
	}
    }
    */
}
