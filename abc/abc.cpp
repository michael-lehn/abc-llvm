#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

#include "expr/implicitcast.hpp"
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

#ifdef ABC_PREFIX
#   define str(s) #s
#   define xstr(s) str(s)
    std::filesystem::path abcPrefix = xstr(ABC_PREFIX);
#   undef str
#   undef xstr
#else
    std::filesystem::path abcPrefix = "/usr/local";
#endif

#ifdef ABC_LIBDIR
#   define str(s) #s
#   define xstr(s) str(s)
    std::filesystem::path abcLibDir = xstr(ABC_LIBDIR);
#   undef str
#   undef xstr
#else
    std::filesystem::path abcLibDir = abcPrefix / "lib";
#endif

#ifdef ABC_INCLUDEDIR
#   define str(s) #s
#   define xstr(s) str(s)
    std::filesystem::path abcIncludeDir = xstr(ABC_INCLUDEDIR);
#   undef str
#   undef xstr
#else
    std::filesystem::path abcIncludeDir = abcPrefix / "include" / "abc";
#endif

void
usage(const char *prog, int exit = 1)
{
    std::cerr << "usage: " << prog << " "
	<< "[ -o outfile ] "
	<< "[ -v ] "
	<< "[ -c | -S | -E | --emit-llvm ] "
	<< "[  -O0 | -O1 | -O2 | -O3 | -Os | -Oz ] "
	<< "[ -Idir... ] "
	<< "[ -Ldir... ] "
	<< "[ -llibrary ] "
	<< "[ -static ] "
	<< "[ -O<level> ] "
	<< "[ -MD -MP -MT <target> -MF <file>] "
	<< "[ --help ] "
	<< "[ --print-ast ] "
	<< "infile..." << std::endl;
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
    bool codegen = true;
    bool createDep = false;
    bool createPhonyDep = false;
    std::filesystem::path depTarget;
    std::filesystem::path depFile;
    bool verbose = false;
    bool staticLink = false;
    llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O0;
    
    for (int i = 1; i < argc; ++i) {
	if (!strcmp(argv[i], "-static")) {
	    staticLink = true;
	} else if (!strcmp(argv[i], "-emit-llvm")) {
	    outputFileType = gen::LLVM_FILE;
	    createExecutable = false;
	} else if (!strncmp(argv[i], "-mmcu=", 6)) {
	    std::string mcu = argv[i] + 6;
	    gen::opt::mcu = mcu;
	} else if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
		case '-':
		    if (!strcmp(argv[i], "--help")) {
			usage(argv[0], 0);
		    } else if (!strcmp(argv[i], "--print-ast")) {
			printAst = true;
		    } else if (!strcmp(argv[i], "--emit-llvm")) {
			outputFileType = gen::LLVM_FILE;
			createExecutable = false;
		    } else if (!strncmp(argv[i], "--target=", 9)) {
			std::string target = argv[i] + 9;
			gen::opt::target = target;
		    } else {
			usage(argv[0], 0);
		    }
		    break;
		case 'O':
		    switch (argv[i][2]) {
			default:
			    usage(argv[0]);
			    break;
			case '0':
			    optLevel = llvm::OptimizationLevel::O0;
			    break;
			case '1':
			    optLevel = llvm::OptimizationLevel::O1;
			    break;
			case '2':
			    optLevel = llvm::OptimizationLevel::O2;
			    break;
			case '3':
			    optLevel = llvm::OptimizationLevel::O3;
			    break;
			case 's':
			    optLevel = llvm::OptimizationLevel::Os;
			    break;
			case 'z':
			    optLevel = llvm::OptimizationLevel::Oz;
			    break;
		    }
		    break;
		case 'v':
		    verbose = true;
		    break;
		case 'c':
		    outputFileType = gen::OBJECT_FILE;
		    createExecutable = false;
		    break;
		case 'E':
		    printAst = true;
		    abc::ImplicitCast::setOutput(false);
		    codegen = false;
		    break;
		case 'S':
		    outputFileType = gen::ASSEMBLY_FILE;
		    createExecutable = false;
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
    abc::lexer::addSearchPath(abcIncludeDir);
    
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
		if (verbose) {
		    std::cerr << asmCmd.c_str() << "\n";
		}
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
	gen::init(infile[i].stem().c_str(), optLevel);
	abc::lexer::init();

	if (!abc::lexer::openInputfile(infile[i].c_str())) {
	    std::cerr << argv[0] << ": error: can not open '"
		<< infile[i].c_str() << "'\n";
	    std::exit(1);
	}
	if (!supportOs.empty()) {
	    abc::lexer::Token macro{abc::lexer::Loc{},
				    abc::lexer::TokenKind::IDENTIFIER,
				    abc::UStr::create(supportOs)};
	    abc::lexer::macro::defineDirective(macro);
	}

	if (verbose) {
	    std::cerr << argv[0];
	    for (const auto &p: abc::lexer::getSearchPath()) {
		std::cerr << " -I " << p;
	    }
	    switch (outputFileType) {
		case gen::ASSEMBLY_FILE:
		    std::cerr << " -S ";
		    break;
		case gen::OBJECT_FILE:
		    std::cerr << " -c ";
		    break;
		case gen::LLVM_FILE:
		    std::cerr << " --emit-llvm ";
		    break;
	    }
	    std::cerr << infile[i].c_str();
	    std::cerr << " -o " << outfile.c_str() << "\n";
	}
	if (auto ast = abc::parser()) {
	    if (printAst) {
		ast->print();
	    }
	    if (codegen) {
		ast->codegen();
		gen::print(outfile.c_str(), outputFileType);
		if (outputFileType == gen::OBJECT_FILE) {
		    objFile.push_back(outfile);
		}
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

    if (codegen && createExecutable) {
	std::string linkerCmd = ccCmd + " -o ";
	linkerCmd += executable.c_str();
	for (const auto &obj: objFile) {
	    linkerCmd += " ";
	    linkerCmd += obj.c_str();
	}
	linkerCmd += ldFlags;
	linkerCmd += " -L ";
	linkerCmd += abcLibDir;
	linkerCmd += " -labc ";
	if (staticLink) {
	    linkerCmd += " -static ";
	}

	if (verbose) {
	    // std::cerr << linkerCmd.c_str() << "\n";
	    auto verbose = linkerCmd + " -### 2>&1 | tail -1";
	    std::system(verbose.c_str());
	}
	if (std::system(linkerCmd.c_str())) {
	    std::cerr << "linker error\n";
	    std::exit(1);
	}
    }
}
