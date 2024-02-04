#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>

#include "gen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "type.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -S | -B] "
	<< "[ -Olevel ] "
	<< "infile" << std::endl;
    std::exit(1);
}

static void
initDefaultTypes()
{
    Symtab::addTypeAlias("char", Type::getChar());
    Symtab::addTypeAlias("void", Type::getVoid());
    Symtab::addTypeAlias("bool", Type::getBool());

    Symtab::addTypeAlias("u8", Type::getUnsignedInteger(8));
    Symtab::addTypeAlias("u16", Type::getUnsignedInteger(16));
    Symtab::addTypeAlias("u32", Type::getUnsignedInteger(32));
    Symtab::addTypeAlias("u64", Type::getUnsignedInteger(64));
    Symtab::addTypeAlias("i8", Type::getSignedInteger(8));
    Symtab::addTypeAlias("i16", Type::getSignedInteger(16));
    Symtab::addTypeAlias("i32", Type::getSignedInteger(32));
    Symtab::addTypeAlias("i64", Type::getSignedInteger(64));

    Symtab::addTypeAlias("int",
			 Type::getSignedInteger(8 * sizeof(int)));
    Symtab::addTypeAlias("long",
			 Type::getSignedInteger(8 * sizeof(long)));
    Symtab::addTypeAlias("long_long",
			 Type::getSignedInteger(8 * sizeof(long long)));
    Symtab::addTypeAlias("unsigned",
			 Type::getUnsignedInteger(8 * sizeof(unsigned)));
    Symtab::addTypeAlias("unsigned_long",
			 Type::getUnsignedInteger(8 * sizeof(unsigned long)));
    Symtab::addTypeAlias("unsigned_long_long",
			 Type::getUnsignedInteger(8 * sizeof(unsigned long long)));
    Symtab::addTypeAlias("size_t",
			 Type::getUnsignedInteger(8 * sizeof(std::size_t)));
    Symtab::addTypeAlias("ptrdiff_t",
			 Type::getSignedInteger(8 * sizeof(std::ptrdiff_t)));
}


int
main(int argc, char *argv[])
{
    const char *infile = nullptr;
    enum Output { ASM = 1, BC = 2 } output = ASM;
    int optLevel = 0;

    initDefaultTypes();

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
