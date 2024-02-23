#include <iostream>
#include <unordered_map>

#include "ustr.hpp"

int
main(void)
{
    auto s1 = UStr::create("for");
    auto s2 = UStr::create("Hallo");

    //kw[UStr{"for"}.c_str] = 1;

    std::cout << "Cmp " << s1.c_str() << " and " << s2.c_str() << " gives "
	<< (s1 == s2) << std::endl;
    /*
    std::cout << "kw 'for'" << kw["for"] << std::endl;
    */
}
