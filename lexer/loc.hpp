#ifndef LEXER_LOC_HPP
#define LEXER_LOC_HPP

#include <cstdint>
#include <ostream>

#include "util/ustr.hpp"

namespace abc { namespace lexer {

class Loc
{
    public:

	class Pos
    	{
	    public:
		Pos() : line{1}, col{1} {}
		Pos(std::size_t line, std::size_t col) : line{line}, col{col} {}
		Pos(const Pos &) = default;
		Pos &operator=(const Pos &) = default;
		std::size_t line, col;
    	};

	Loc() = default;
	Loc(UStr path, Pos from, Pos to) : path{path}, from{from}, to{to} {}
	Loc(const Loc &loc) = default;
	Loc &operator=(const Loc &loc) = default;
	Loc &operator=(Loc &&loc) = default;

	UStr path;
	Pos from, to;
};

std::ostream &operator<<(std::ostream &out, Loc::Pos pos);
std::ostream &operator<<(std::ostream &out, Loc loc);

} } // namespace lexer, abc

#endif // LEXER_LOC_HPP
