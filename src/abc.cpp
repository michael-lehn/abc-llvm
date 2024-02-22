#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>

#include "asserthack.hpp"
#include "gen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "symtab.hpp"
#include "type.hpp"

void
usage(const char *prog)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -c | -S | -B] "
	<< "[ -Idir... ] "
	<< "[ -Olevel ] "
	<< "infile" << std::endl;
    std::exit(1);
}

static void
initDefaultTypes()
{
    // assert hack
    asserthack::makeDecl();

    Symtab::addTypeAlias(
	    "char",
	    Type::createAlias(
		"char",
		Type::getChar()));

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

    Symtab::addTypeAlias(
		"int",
		Type::createAlias(
		    "int",
		    Type::getSignedInteger(8 * sizeof(int))));
    Symtab::addTypeAlias(
		"long",
		Type::createAlias(
		    "long",
		    Type::getSignedInteger(8 * sizeof(long))));
    Symtab::addTypeAlias(
		"long_long",
		Type::createAlias(
		    "long_long",
		    Type::getSignedInteger(8 * sizeof(long long))));
    Symtab::addTypeAlias(
		"unsigned",
		 Type::createAlias(
		     "unsigned",
		     Type::getUnsignedInteger(8 * sizeof(unsigned))));
    Symtab::addTypeAlias(
		"unsigned_long",
		 Type::createAlias(
		     "unsigned_long",
		     Type::getUnsignedInteger(8 * sizeof(unsigned long))));
    Symtab::addTypeAlias(
		"unsigned_long_long",
		 Type::createAlias(
		     "unsigned_long_long",
		     Type::getUnsignedInteger(8 * sizeof(unsigned long long))));
    Symtab::addTypeAlias(
		"size_t",
		 Type::createAlias(
		     "size_t",
		     Type::getUnsignedInteger(8 * sizeof(std::size_t))));
    Symtab::addTypeAlias(
		"ptrdiff_t",
		 Type::createAlias(
		     "ptrdiff_t",
		     Type::getSignedInteger(8 * sizeof(std::ptrdiff_t))));
}

void
optInclude(const char *dir)
{
    addIncludePath(dir);
}

int
main(int argc, char *argv[])
{
    enum Output { ASM = 1, OBJ = 2, EXE = 3, BC = 4 } output = EXE;
    int optLevel = 0;
    std::filesystem::path outfile;

    std::vector<std::filesystem::path> infile;
    std::vector<std::filesystem::path> objfile;


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
		case 'c':
		    output = OBJ;
		    break;
		case 'S':
		    output = ASM;
		    break;
		case 'B':
		    output = BC;
		    break;
		case 'I':
		    if (!argv[i][2] && i + 1 < argc) {
			optInclude(argv[i + 1]);
			++i;
		    } else if (argv[i][2]) {
			optInclude(&argv[i][2]);
		    } else {
			usage(argv[0]);
		    }
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
	} else {
	    infile.push_back(argv[i]);
	}
    }
    if (!infile.size()) {
	usage(argv[0]);
    }

    if (output != EXE && !outfile.empty() && infile.size() > 1) {
	std::cerr << argv[0] << ": error: "
	    << "cannot specify -o when generating multiple output files\n";
	std::exit(1);
    }

    for (auto in: infile) {
	auto out = infile.size() > 1 || outfile.empty()
	    ? in
	    : outfile;
	if (!setLexerInputfile(in.c_str())) {
	    std::cerr << "can not read '" << in.c_str() << "'\n";
	}
	gen::setTarget(optLevel);

	Symtab::openScope();
	initDefaultTypes();
	if (auto ast = parser()) {
	    // ast->print();
	    ast->codegen();
	} else {
	    Symtab::closeScope();
	    std::exit(1);
	}
	Symtab::closeScope();
	switch (output) {
	    case ASM:
		out.replace_extension(".s");
		break;
	    case OBJ:
	    case EXE:
		out.replace_extension(".o");
		break;
	    case BC:
		out.replace_extension(".bc");
		break;
	    default:
		assert(0);
	}
	if (output == ASM) {
	    gen::dump_asm(out, optLevel);
	} else if (output == OBJ || output == EXE) {
	    gen::dump_obj(out, optLevel);
	} else {
	    gen::dump_bc(out);
	}

	if (output) {
	    objfile.push_back(out);
	}
    }
    if (output == EXE) {
	std::string linker = "cc -o ";
	linker += outfile.empty() ? "a.out" : outfile.c_str();

	for (auto obj: objfile) {
	    linker = linker + " " + obj.c_str();
	}
	if (std::system(linker.c_str())) {
	    std::cerr << "linker error\n";
	    std::exit(1);
	}
    }
}
