#include <string>

#include "llvm/ADT/edit_distance.h"

#include "editdistance.hpp"

namespace gen {

unsigned
editDistance(const std::string &s1, const std::string &s2)
{
    llvm::ArrayRef<char> s1_{s1.data(), s1.length()};
    llvm::ArrayRef<char> s2_{s2.data(), s2.length()};

    return llvm::ComputeEditDistance(s1_, s2_);
}

} // namespace gen
