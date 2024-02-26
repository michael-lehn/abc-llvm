#ifndef LOC_HPP
#define LOC_HPP

#include <ostream>

#include "ustr.hpp"

namespace lexer {

class Loc
{
    public:

	class Pos
    	{
	    public:
		Pos() : line{0}, col{0} {}
		Pos(std::size_t line, std::size_t col) : line{line}, col{col} {}
		Pos(const Pos &) = default;
		Pos &operator=(const Pos &) = default;
		std::size_t line, col;
    	};

	Loc() = default;
	Loc(UStr path, Pos from, Pos to) : path{path}, from{from}, to{to} {}
	Loc(const Loc &loc) = default;
	Loc &operator=(const Loc &loc) = delete;

	const UStr path;
	const Pos from, to;
};

std::ostream &operator<<(std::ostream &out, Loc::Pos pos);
std::ostream &operator<<(std::ostream &out, Loc loc);

} // namespace lexer

#endif // LOC_HPP
