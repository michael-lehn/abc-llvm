#include <iostream>
#include <system_error>

#include "gen.hpp"
#include "print.hpp"

namespace gen {

void
print(std::filesystem::path path)
{
    assert(llvmContext);
    std::error_code ec;
    path.replace_extension("bc");
    auto f = llvm::raw_fd_ostream (path.c_str(), ec);

    if (ec) {
	llvm::errs() << "Could not open file: " << ec.message();
	std::exit(1);
    }
    llvmModule->print(f, nullptr);
}

} // namespace gen
