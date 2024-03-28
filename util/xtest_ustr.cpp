#include <iostream>

#include "ustr.hpp"

int
main()
{
    auto s = abc::UStr::create("some string");

    std::cerr << "s = " << s << "\n";
}
