#include <set>

#include "ustr.hpp"

namespace abc {

static std::set<std::string> ustrSet;

UStr::UStr()
    : c_str_{nullptr}, len{0}
{
}

UStr::UStr(const std::string &s)
    : c_str_{ustrSet.insert(s).first->c_str()}
    , len{s.length()} 
{
}

void
UStr::init()
{
    ustrSet.clear();
}

UStr
UStr::create(const char *s)
{
    return UStr{s};
}

UStr
UStr::create(const std::string &s)
{
    return UStr{s};
}

std::ostream &
operator<<(std::ostream &out, const UStr &ustr)
{
    if (ustr.c_str()) {
	out << ustr.c_str();
    }
    return out;
}

} // namespace abc
