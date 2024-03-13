#ifndef GEN_PRINT_HPP
#define GEN_PRINT_HPP

#include <filesystem>

namespace gen {

enum FileType
{
    ASSEMBLY_FILE,
    OBJECT_FILE,
    LLVM_FILE,
};

void print(std::filesystem::path path, FileType fileType = LLVM_FILE);

} // namespace gen

#endif // GEN_PRINT_HPP
