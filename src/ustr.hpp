#ifndef USTR_HPP
#define USTR_HPP

#include <functional>
#include <set>
#include <string>

class UStr
{
    public:
	UStr(void);
	UStr(const char *s);
	UStr(const std::string &s);

	const char *
	c_str(void) const
	{
	    return c_str_;
	}

    private:
	const char *c_str_;
};

inline bool
operator==(const UStr &a, const UStr &b)
{
    return a.c_str() == b.c_str();
}

inline bool
operator<(const UStr &a, const UStr &b)
{
    return a.c_str() < b.c_str();
}


template<>
struct std::hash<UStr>
{
    std::size_t operator()(const UStr& s) const noexcept
    {
        return std::hash<const char *>{}(s.c_str());
    }
};

#endif // USTR_HPP
