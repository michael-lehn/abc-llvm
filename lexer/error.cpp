#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

#include "error.hpp"
#include "lexer.hpp"

namespace abc { namespace error {

std::ostream &
out(int indent)
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    return std::cerr;
}

void
fatal()
{
    std::exit(1);
}

void
warning()
{
    out() << std::endl << "WARNING" << std::endl << std::endl;
}

bool
expected(lexer::TokenKind kind)
{
    if (lexer::token.kind != kind) {
	std::cerr << "lexer::token.kind = " << lexer::token.kind << "\n";
	error::location(lexer::token.loc);
	out() << error::setColor(error::BOLD) << lexer::token.loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "expected '" << kind << "'\n"
	    << error::setColor(error::NORMAL);
	fatal();
	return false;
    }
    return true;
}

static std::unordered_map<Color, std::string> colorMap = {
    { NORMAL, "\033[0m"},
    { BOLD, "\033[0m" "\033[1;10m"},
    { RED, "\033[0;31m"},
    { BLUE, "\033[0;34m"},
    { BOLD_RED, "\033[1;31m"},
    { BOLD_BLUE, "\033[1;31m"},
};

std::string
setColor(Color color)
{
    return colorMap.at(color);
}

static std::string
expandTabs(const std::string &str)
{
    std::string result;
    std::size_t pos = 0;
    constexpr unsigned tabSize = 8;

    for(char c: str) {
        if(c == '\t') {
            result.append(tabSize - pos % tabSize, ' ');
            pos = 0;
        } else {
            result += c;
            pos = (c == '\n') ? 0 : pos + 1;
        }
    }
    return result;
}

static std::pair <std::size_t, std::size_t>
printLine(std::ostream &out, const char *path, std::size_t lineNumber)
{
    std::fstream file{path};

    file.seekg(std::ios::beg);
    for (std::size_t i = 0; i + 1 < lineNumber; ++i) {
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    std::string line;
    std::getline(file, line);
    line = expandTabs(line);
    out << line << std::endl;
    return {line.find_first_not_of(' '), line.length()};
}

std::ostream &
location(const lexer::Loc &loc)
{
    auto &out = std::cerr;

    out << std::endl;
    for (std::size_t line = loc.from.line; line <= loc.to.line; ++line) {
	auto [from, len] = printLine(out, loc.path.c_str(), line);
	std::size_t fromCol = line == loc.from.line ? loc.from.col : from + 1;
	std::size_t toCol = line == loc.to.line ? loc.to.col : len;
	for (size_t i = 1; i <= toCol; ++i) {
	    if (i < fromCol) {
		out << " ";
	    } else {
		out << "^";
	    }
	}
	out << std::endl;
    }
    return out;
}

} } // namespace error, abc
