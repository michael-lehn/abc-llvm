#include <set>

#include "ustr.hpp"

static std::set<std::string> *ustrSet;

UStr::UStr()
    : c_str_{nullptr}, len{0}
{
}

UStr::UStr(const char *s)
    : UStr{std::string{s}}
{
}

UStr::UStr(const std::string &s)
    : c_str_{
	(ustrSet ? ustrSet
		 : ustrSet = new std::set<std::string>
		 )->insert(s).first->c_str()
	}
    , len{s.length()} 
{
}
