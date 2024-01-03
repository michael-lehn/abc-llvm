#include <set>

#include "ustr.hpp"

static std::set<std::string> *ustrSet;

UStr::UStr(void)
    : c_str_{nullptr}
{
}

UStr::UStr(const char *s)
    : c_str_{
	(ustrSet ? ustrSet
		 : ustrSet = new std::set<std::string>
		 )->insert(s).first->c_str()
    }
{
}

UStr::UStr(const std::string &s)
    : c_str_{
	(ustrSet ? ustrSet
		 : ustrSet = new std::set<std::string>
		 )->insert(s).first->c_str()
    }
{
}
