#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

#include "gen/gen.hpp"
#include "gen/print.hpp"
#include "lexer/lexer.hpp"
#include "lexer/macro.hpp"
#include "lexer/reader.hpp"
#include "parser/parser.hpp"
#include "type/inittypesystem.hpp"

#ifdef SUPPORT_CC
#   define str(s) #s
#   define xstr(s) str(s)
    std::string ccCmd = xstr(SUPPORT_CC);
#   undef str
#   undef xstr
#else
    std::string ccCmd = "cc";
#endif // SUPPORT_CC

#ifdef SUPPORT_OS
#   define str(s) #s
#   define xstr(s) str(s)
    std::string supportOs = xstr(SUPPORT_OS);
#   undef str
#   undef xstr
#else
    std::string supportOs;
#endif // SUPPORT_OS

void
usage(const char *prog, int exit = 1)
{
    std::cerr << "usage: " << prog
	<< "[ -o outfile ] "
	<< "[ -c | -S | --emit-llvm ] "
	<< "[ -Idir... ] "
	<< "[ -Ldir... ] "
	<< "[ -llibrary ] "
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
    std::vector<std::filesystem::path> infile;
    std::filesystem::path outfile;
    bool createExecutable = true;
    std::filesystem::path executable = "a.out";
    std::vector<std::filesystem::path> objFile;
    gen::FileType outputFileType = gen::OBJECT_FILE;
    std::string ldFlags;
    bool printAst = false;
    int optimizationLevel = 0;
    bool createDep = false;
    bool createPhonyDep = false;
    std::filesystem::path depTarget;
    std::filesystem::path depFile;
    
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
			abc::lexer::addSearchPath(argv[i + 1]);
			++i;
		    } else if (argv[i][2]) {
			abc::lexer::addSearchPath(&argv[i][2]);
		    } else {
			usage(argv[0]);
		    }
		    break;
		case 'l':
		    ldFlags += " -l";
		    if (!argv[i][2] && i + 1 < argc) {
			ldFlags += argv[i + 1];
			++i;
		    } else if (argv[i][2]) {
			ldFlags += &argv[i][2];
		    } else {
			usage(argv[0]);
		    }
		    break;
		case 'L':
		    ldFlags += " -L";
		    if (!argv[i][2] && i + 1 < argc) {
			ldFlags += argv[i + 1];
		    } else if (argv[i][2]) {
			ldFlags += &argv[i][2];
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
	    infile.push_back(argv[i]);
	}
    }
    if (infile.empty()) {
	std::cerr << argv[0] << ": error: no input files\n";
	std::exit(1);
    }
    if (!outfile.empty()) {
	if (createExecutable) {
	    executable = outfile;
	    outfile.clear();
	} else if (infile.size() > 1) {
	    std::cerr << argv[0] << ": error: cannot specify ";
	    switch (outputFileType) {
		case gen::LLVM_FILE:
		    std::cerr << "--emit-llvm";
		    break;
		case gen::OBJECT_FILE:
		    std::cerr << "-o";
		    break;
		case gen::ASSEMBLY_FILE:
		    std::cerr << "-S";
		    break;
	    }
	    std::cerr << " when generating multiple output files\n";
	    std::exit(1);
	}
    }

    bool useDefaultOutfile = outfile.empty();
    for (std::size_t i = 0; i < infile.size(); ++i) {
	if (useDefaultOutfile) {
	    switch (outputFileType) {
		case gen::ASSEMBLY_FILE:
		    outfile = infile[i].filename().replace_extension("s");
		    break;
		case gen::OBJECT_FILE:
		    outfile = infile[i].filename().replace_extension("o");
		    break;
		case gen::LLVM_FILE:
		    outfile = infile[i].filename().replace_extension("ll");
		    break;
		default:
		    assert(0);
		    break;
	    }
	}
	if (createExecutable) {
	    outfile = std::filesystem::temp_directory_path() / outfile;
	}
	if (infile[i].extension() == ".o") {
	    objFile.push_back(infile[i]);
	    continue;
	}
	if (infile[i].extension() == ".s") {
	    if (outputFileType == gen::LLVM_FILE) {
		std::cerr << argv[0] << ": error: can not convert "
		    << infile[i] << " to " << outfile << "\n";
		std::exit(1);
	    } else if (outputFileType == gen::OBJECT_FILE) {
		std::string asmCmd = ccCmd + " -c -o ";
		asmCmd +=  outfile.c_str();
		asmCmd += " ";
		asmCmd += infile[i].c_str();
		if (std::system(asmCmd.c_str())) {
		    std::exit(1);
		}
		objFile.push_back(outfile);
		continue;
	    }
	    continue;
	}
	if (infile[i].extension() != ".abc") {
	    ldFlags += " ";
	    ldFlags += infile[i];
	    continue;
	}

	abc::initTypeSystem();
	gen::init(infile[i].stem().c_str(), optimizationLevel);
	abc::lexer::init();

	if (!abc::lexer::openInputfile(infile[i].c_str())) {
	    std::cerr << argv[0] << ": error: can not open '"
		<< infile[i].c_str() << "'\n";
	    std::exit(1);
	}
	if (!supportOs.empty()) {
	    abc::lexer::macro::defineDirective(abc::UStr::create(supportOs));
	}

	if (auto ast = abc::parser()) {
	    if (printAst) {
		ast->print();
	    }
	    ast->codegen();
	    gen::print(outfile.c_str(), outputFileType);
	    if (outputFileType == gen::OBJECT_FILE) {
		objFile.push_back(outfile);
	    }
	} else {
	    std::exit(1);
	}

	if (createDep) {
	    if (depFile.empty()) {
		depFile = infile[i].stem().replace_extension("d");
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
	    fs << depTarget.c_str() << ": " << infile[i].c_str() << " ";
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

    if (createExecutable) {
	std::string linker = ccCmd + " -o ";
	linker += executable.c_str();
	for (const auto &obj: objFile) {
	    linker += " ";
	    linker += obj.c_str();
	}
	linker += ldFlags;

	if (std::system(linker.c_str())) {
	    std::cerr << "linker error\n";
	    std::exit(1);
	}
    }
}
