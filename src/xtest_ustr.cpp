#include <iostream>
#include <unordered_map>

#include "ustr.hpp"

std::unordered_map<UStr, int> kw = {
    {"for", 1},
};

int
main(void)
{
    UStr s1 = "for";
    UStr s2 = "Hallo";

    //kw[UStr{"for"}.c_str] = 1;

    std::cout << "Cmp " << s1.c_str() << " and " << s2.c_str() << " gives "
	<< (s1 == s2) << std::endl;
    /*
    std::cout << "kw 'for'" << kw["for"] << std::endl;
    */
    std::cout << "kw[" << s1.c_str() << "] = " << kw[s1] << std::endl;
}
