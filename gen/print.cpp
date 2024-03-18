#include <iostream>
#include <system_error>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"

#include "gen.hpp"
#include "print.hpp"

namespace gen {

void
print(std::filesystem::path path, FileType fileType)
{
    assert(llvmContext);
    assert(targetMachine);
    std::error_code ec;
    auto f = llvm::raw_fd_ostream{path.c_str(), ec, llvm::sys::fs::OF_None};

    if (ec) {
	llvm::errs() << "Could not open file: " << ec.message();
	std::exit(1);
    }

    if (fileType == LLVM_FILE) {
	llvmModule->print(f, nullptr);
	return;
    }

    llvm::legacy::PassManager pass;
    auto llvmFileType = fileType == OBJECT_FILE
#if LLVM_MAJOR_VERSION >= 18
        ? llvm::CodeGenFileType::ObjectFile
        : llvm::CodeGenFileType::AssemblyFile;
#else
	? llvm::CodeGenFileType::CGFT_ObjectFile
	: llvm::CodeGenFileType::CGFT_AssemblyFile;
#endif


    if (targetMachine->addPassesToEmitFile(pass, f, nullptr, llvmFileType)) {
	llvm::errs() << "can't emit a file of this type";
	std::exit(1);
    }
    pass.run(*llvmModule);
    f.flush();
}

} // namespace gen
