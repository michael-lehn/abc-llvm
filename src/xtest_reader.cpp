#include <cstdio>
#include <filesystem>
#include <iostream>

#include "reader.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -Idir... ] "
	<< "[ infile ]" << std::endl;
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
			lexer::addSearchPath(argv[i + 1]);
			++i;
		    } else if (argv[i][2]) {
			lexer::addSearchPath(&argv[i][2]);
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
	? lexer::openInputfile(nullptr)
	: lexer::openInputfile(infile.c_str());

    do {
	lexer::nextCh();
	std::cerr
	    << lexer::reader->path << ":"
	    << lexer::reader->pos << ": "
	    << "'" << char(lexer::reader->ch) << "'\n";
	if (lexer::reader->ch == '\n') {
	    std::cerr << "val = '" << lexer::reader->val << "'\n";
	    lexer::reader->resetStart();
	}
    } while (!lexer::reader->eof());
}
