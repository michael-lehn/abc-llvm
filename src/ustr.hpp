#ifndef USTR_HPP
#define USTR_HPP

#include <functional>
#include <set>
#include <string>

class UStr
{
    public:
	UStr();
	UStr(const UStr &) = default;

    protected:	
	UStr(const std::string &s);

    public:

	static UStr create(const char *s);
	static UStr create(const std::string &s);

	UStr &operator=(const UStr &) = default;

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

    private:
	const char *c_str_;
	std::size_t len;
};

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


template<>
struct std::hash<UStr>
{
    std::size_t operator()(const UStr& s) const noexcept
    {
        return std::hash<const char *>{}(s.c_str());
    }
};

#endif // USTR_HPP
