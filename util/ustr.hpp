#ifndef UTIL_USTR_HPP
#define UTIL_USTR_HPP

#include <cstdint>
#include <functional>
#include <ostream>
#include <set>
#include <string>

namespace abc {

class UStr
{
    public:
	UStr();
	UStr(const UStr &) = default;

    protected:	
	UStr(const std::string &s);

    public:

	static void init();
	static UStr create(const char *s);
	static UStr create(const std::string &s);

	UStr &operator=(const UStr &) = default;
	UStr &operator=(UStr &&) = default;

	const char *
	c_str() const
	{
	    return c_str_;
	}

	std::size_t
	length() const
	{
	    return len;
	}

	bool
	empty() const
	{
	    return length() == 0;
	}

    private:
	const char *c_str_;
	std::size_t len;
};

std::ostream &operator<<(std::ostream &out, const UStr &ustr);

inline bool
operator==(const UStr &a, const UStr &b)
{
    return a.c_str() == b.c_str();
}

inline bool
operator!=(const UStr &a, const UStr &b)
{
    return a.c_str() != b.c_str();
}

inline bool
operator<(const UStr &a, const UStr &b)
{
    return a.c_str() < b.c_str();
}

} // namespace abc

template<>
struct std::hash<abc::UStr>
{
    std::size_t operator()(const abc::UStr& s) const noexcept
    {
        return std::hash<const char *>{}(s.c_str());
    }
};


#endif // UTIL_USTR_HPP
