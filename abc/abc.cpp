#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "lexer/macro.hpp"
#include "lexer/reader.hpp"
#include "parser/parser.hpp"


void
usage(const char *prog, int exit = 1)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -c | -S | --emit-llvm ] "
	<< "[ -Idir... ] "
	<< "[ -O<level> ] "
	<< "[ -MD -MP -MT <target> -MF <file>] "
	<< "[ --help ] "
	<< "[ --print-ast ] "
	<< "infile" << std::endl;
    std::exit(exit);
}

int
main(int argc, char *argv[])
{
    std::filesystem::path infile;
    std::filesystem::path outfile;
    bool createExecutable = true;
    std::filesystem::path executable = "a.out";
    gen::FileType outputFileType = gen::OBJECT_FILE;
    bool printAst = false;
    int optimizationLevel = 0;
    bool createDep = false;
    bool createPhonyDep = false;
    std::filesystem::path depTarget;
    std::filesystem::path depFile;

#ifdef SUPPORT_OS
#   define str(s) #s
#   define xstr(s) str(s)
    abc::lexer::macro::defineDirective(abc::UStr::create(xstr(SUPPORT_OS)));
#   undef str
#   undef xstr
#endif // SUPPORT_OS

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
			createExecutable = false;
		    }
		    break;
		case 'c':
		    outputFileType = gen::OBJECT_FILE;
		    createExecutable = false;
		    break;
		case 'S':
		    outputFileType = gen::ASSEMBLY_FILE;
		    createExecutable = false;
		    break;
		case 'O':
		    optimizationLevel = 3;
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
		case 'M':
		    switch (argv[i][2]) {
			default:
			    usage(argv[0]);
			    break;
			case 'D':
			    createDep = true;
			    break;
			case 'P':
			    createPhonyDep = true;
			    break;
			case 'T':
			    if (i + 1 < argc) {
				depTarget = argv[++i];
			    } else {
				usage(argv[0]);
			    }
			    break;
			case 'F':
			    if (i + 1 < argc) {
				depFile = argv[++i];
			    } else {
				usage(argv[0]);
			    }
			    break;
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
    if (createExecutable && !outfile.empty()) {
	executable = outfile;
	outfile.clear();
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
    if (createExecutable) {
	outfile = std::filesystem::temp_directory_path() / outfile;
    }

    if (!abc::lexer::openInputfile(infile.c_str())) {
	std::cerr << argv[0] << ": error: can not open '" << infile.c_str()
	    << "'\n";
	return 1;
    }
    abc::lexer::init();
    gen::init(infile.stem().c_str(), optimizationLevel);

    if (auto ast = abc::parser()) {
	if (printAst) {
	    ast->print();
	}
	ast->codegen();
	gen::print(outfile.c_str(), outputFileType);
    } else {
	std::exit(1);
    }

    if (createExecutable) {
#ifdef SUPPORT_CC
#   define str(s) #s
#   define xstr(s) str(s)
	std::string linker = xstr(SUPPORT_CC) " -o ";
#   undef str
#   undef xstr
#else
	std::string linker = "cc -o ";
#endif // SUPPORT_CC
	linker += executable.c_str();
	linker += " ";
	linker += outfile.c_str();

	if (std::system(linker.c_str())) {
	    std::cerr << "linker error\n";
	    std::exit(1);
	}
    }

    if (createDep) {
	if (depFile.empty()) {
	    depFile = infile.stem().replace_extension("d");
	}
	std::fstream fs;
	fs.open(depFile, std::ios::out);
	if (!fs.good()) {
	    std::cerr << "Could not open file: " << depFile << "\n";
	    std::exit(1);
	}
	if (depTarget.empty()) {
	    depTarget = outfile;
	}
	fs << depTarget.c_str() << ": " << infile.c_str() << " ";
	for (const auto &file: abc::lexer::includedFiles()) {
	    fs << file.c_str() << " ";
	}
	fs << "\n";
	if (createPhonyDep) {
	    for (const auto &file: abc::lexer::includedFiles()) {
		fs << file.c_str() << ":\n";
	    }
	}
    }
}
